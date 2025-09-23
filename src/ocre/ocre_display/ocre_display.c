/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <zephyr/logging/log.h>

#include <ocre/ocre.h>
#include <ocre/api/ocre_common.h>

#include "ocre_display.h"
#include "wasm_export.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/input/input.h>

#include "wasm_memory.h"

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#define MONITOR_HOR_RES 480
#define MONITOR_VER_RES 272

#define TOUCH_EVENT_QUEUE_SIZE 8

static char *lcd_get_pixel_format_str(enum display_pixel_format pix_fmt)
{
    switch (pix_fmt) {
        case PIXEL_FORMAT_ARGB_8888: return "ARGB8888";
        case PIXEL_FORMAT_RGB_888:   return "RGB888";
        case PIXEL_FORMAT_RGB_565:   return "RGB565";
        case PIXEL_FORMAT_BGR_565:   return "BGR565";
        case PIXEL_FORMAT_L_8:       return "L8";
        case PIXEL_FORMAT_MONO01:    return "MONO01";
        case PIXEL_FORMAT_MONO10:    return "MONO10";
        default:                     return "Unknown";
    }
}

static int lcd_initialized = 0;
static const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct device *const touch_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_touch));

static void *fb = NULL;
static wasm_shared_heap_t _fb_heap;

static uint8_t g_bpp = 0;
struct touch_evt {
    uint16_t x;
    uint16_t y;
    uint8_t  pressed; // 0/1
};

K_MSGQ_DEFINE(touch_q, sizeof(struct touch_evt), TOUCH_EVENT_QUEUE_SIZE, 4);

static struct touch_evt last_evt = {0, 0, 0};

static void touch_event_callback(struct input_event *evt, void *user_data)
{
    static uint16_t x, y;
    static uint8_t pressed;

    if (evt->code == INPUT_ABS_X) {
        x = (uint16_t)evt->value;
    } else if (evt->code == INPUT_ABS_Y) {
        y = (uint16_t)evt->value;
    } else if (evt->code == INPUT_BTN_TOUCH) {
        pressed = evt->value ? 1 : 0;
    }

    if (evt->sync) {
        struct touch_evt e = { .x = x, .y = y, .pressed = pressed };

        /* Skip duplicates */
        if (e.x == last_evt.x && e.y == last_evt.y && e.pressed == last_evt.pressed) {
            LOG_DBG("Skipping duplicate touch event");
            return;
        }

        if (k_msgq_put(&touch_q, &e, K_NO_WAIT) == 0) {
            last_evt = e;  /* Update only when successfully queued */
        } else {
            LOG_WRN("Touch event queue full, dropping event");
        }
    }
}

// Requires CONFIG_INPUT_MODE_SYNCHRONOUS=y
INPUT_CALLBACK_DEFINE(touch_dev, touch_event_callback, NULL);

// LVGL supports LV_COLOR_DEPTH of 1, 4, 8, 16, or 32
// Supporting 16 and 32 only for now
void display_set_runtime_bpp_from_caps(const struct display_capabilities *caps)
{
    switch (caps->current_pixel_format) {
        case PIXEL_FORMAT_RGB_565:
        case PIXEL_FORMAT_BGR_565:
            g_bpp = 2;
            break;
        case PIXEL_FORMAT_RGB_888:
        case PIXEL_FORMAT_ARGB_8888:
            g_bpp = 4;
            break;
        default:
            LOG_ERR("Unsupported display pixel format %s (%d), cannot continue",
                    lcd_get_pixel_format_str(caps->current_pixel_format),
                    caps->current_pixel_format);
            g_bpp = 0;
            break;
    }
}

void ocre_display_init(void) {

    if (lcd_initialized != 0) {
        return;
    }
    
    LOG_INF("Initializing display %s", display_dev->name);
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device %s not ready", display_dev->name);
        return;
    }

    LOG_INF("Initializing touch device %s", touch_dev->name);
    if (!device_is_ready(touch_dev)) {
        LOG_ERR("Touch device %s not ready", touch_dev->name);
        return;
    }

    // Backlight off, clear display
    display_set_pixel_format(display_dev, PIXEL_FORMAT_BGR_565);
    display_blanking_on(display_dev);
    display_clear(display_dev);

    // Query display capabilities
    struct display_capabilities capabilities;
	display_get_capabilities(display_dev, &capabilities);
    LOG_INF("Display capabilities:");
    LOG_INF("\tResolution: %dx%d", capabilities.x_resolution, capabilities.y_resolution);
    LOG_INF("\tPixel format: %s (%d)", lcd_get_pixel_format_str(capabilities.current_pixel_format), capabilities.current_pixel_format);
    LOG_INF("\tOrientation: %d", capabilities.current_orientation);

    // Set internal bpp based on pixel format
    display_set_runtime_bpp_from_caps(&capabilities);
    LOG_INF("\tBPP: %d", g_bpp);

    // // Calculate framebuffer size
    // uint32_t fb_size = (uint32_t)capabilities.x_resolution * (uint32_t)capabilities.y_resolution * (uint32_t)g_bpp;
    // LOG_INF("Calculated framebuffer size: %d bytes", fb_size);

    // // FOR NXP RT1064:
    // // 261120 bytes + 512 bytes overhead from NXP driver = 261632
    // // However must be aligned to system page size of 4096, so round up to 262144
    // uint32_t aligned_size = (fb_size + 4095) & ~4095;
    // LOG_INF("Aligned size %u", aligned_size);

    // // void *fb = display_get_framebuffer(display_dev);
    // fb = malloc(aligned_size);
    // if (fb) {
    //     LOG_INF("Display framebuffer address: %p", fb);
    // } else {
    //     LOG_WRN("Could not get display framebuffer. Shared heap will be unavailable.");
    //     return;
    // }

    // // Create shared heap for framebuffer
    // SharedHeapInitArgs fb_heap_init_args;
    // memset(&fb_heap_init_args, 0, sizeof(SharedHeapInitArgs));
    // fb_heap_init_args.pre_allocated_addr = fb;    
    // fb_heap_init_args.size = aligned_size; // should be 262144
    // _fb_heap = wasm_runtime_create_shared_heap(&fb_heap_init_args);
    // if (!_fb_heap) {
    //     LOG_ERR("Failed to create shared heap for framebuffer");
    //     _fb_heap = NULL;
    //     free(fb);
    //     return;
    // }

    LOG_INF("ocre_display_init: OK");
    lcd_initialized = 1;
}

wasm_shared_heap_t ocre_display_get_shared_heap(void)
{
    return (lcd_initialized == 1) ? _fb_heap : NULL;
}

void
display_init(void)
{
    display_blanking_off(display_dev);    
}

void
display_flush(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
              int32_t y2, lv_color_t *color)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    struct display_buffer_descriptor desc;

    if (g_bpp == 0 || x2 < x1 || y2 < y1)
        return;

    /* keep your current minimal validation */
    if (!wasm_runtime_validate_native_addr(module_inst, color,
                                           (uint64_t)sizeof(lv_color_t)))
        return;

    uint16_t w = (uint16_t)(x2 - x1 + 1);
    uint16_t h = (uint16_t)(y2 - y1 + 1);

    desc.buf_size = (uint32_t)g_bpp * (uint32_t)w * (uint32_t)h;
    desc.width = w;
    desc.pitch = w;
    desc.height = h;

    display_write(display_dev, (uint16_t)x1, (uint16_t)y1, &desc, (void *)color);
}

void display_input_read(wasm_exec_env_t exec_env,
                        int32_t *x, int32_t *y,
                        bool *pressed, bool *more)
{
    struct touch_evt e;

    if (k_msgq_get(&touch_q, &e, K_NO_WAIT) == 0) {
        // Got a new event
        *x = e.x;
        *y = e.y;
        *pressed = e.pressed;
        *more = k_msgq_num_used_get(&touch_q) > 0;
    } else {
        // No new event: use last known state
        *x = last_evt.x;
        *y = last_evt.y;
        *pressed = last_evt.pressed;
        *more = false;
    }
}

void
display_deinit(wasm_exec_env_t exec_env)
{}

int
time_get_ms(wasm_exec_env_t exec_env)
{
    return k_uptime_get_32();
}