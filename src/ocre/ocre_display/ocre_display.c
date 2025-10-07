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

// LCD display device
static int lcd_initialized = 0;
static const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static uint8_t g_bpp = 0;
static uint32_t g_width = 0;
static uint32_t g_height = 0;
static ocre_color_mode_t g_color_mode = COLOR_MODE_UNKNOWN;

// Touch device
#define TOUCH_EVENT_QUEUE_SIZE 8
static const struct device *const touch_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_touch));
struct touch_evt {
    uint16_t x;
    uint16_t y;
    uint8_t  pressed; // 0/1
};
static struct touch_evt last_evt = {0, 0, 0};
K_MSGQ_DEFINE(touch_q, sizeof(struct touch_evt), TOUCH_EVENT_QUEUE_SIZE, 4);

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

static int32_t display_set_runtime_bpp_from_caps(const struct display_capabilities *caps)
{
    switch (caps->current_pixel_format) {
        case PIXEL_FORMAT_RGB_565:   return 2;
        case PIXEL_FORMAT_BGR_565:   return 2;
        case PIXEL_FORMAT_RGB_888:   return 3;
        case PIXEL_FORMAT_ARGB_8888: return 4;
        default:                     return 0;
    }
}

void ocre_display_init_internal(void) {

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
    display_blanking_on(display_dev);
    display_clear(display_dev);

    // Query display capabilities
    struct display_capabilities capabilities;
	display_get_capabilities(display_dev, &capabilities);

    LOG_INF("Display capabilities:");
    LOG_INF("\tResolution: %dx%d", capabilities.x_resolution, capabilities.y_resolution);
    g_width = capabilities.x_resolution;
    g_height = capabilities.y_resolution;

    // Currently we are only handling 16bpp formats
    if (capabilities.supported_pixel_formats & PIXEL_FORMAT_RGB_565) {
        display_set_pixel_format(display_dev, PIXEL_FORMAT_RGB_565);
        g_color_mode = COLOR_MODE_RGB565;
        LOG_INF("\tPixel mode: RGB565");
    } else if (capabilities.supported_pixel_formats & PIXEL_FORMAT_BGR_565) {
        display_set_pixel_format(display_dev, PIXEL_FORMAT_BGR_565);
        g_color_mode = COLOR_MODE_BGR565;
        LOG_INF("\tPixel mode: BGR565");
    } else {
        g_color_mode = COLOR_MODE_UNKNOWN;
        LOG_ERR("\tPixel mode: Unknown (only RGB565 or BGR565 supported for now)");
        return;
    }  

    LOG_INF("\tOrientation: %d", capabilities.current_orientation);

    // Set internal bpp based on pixel format
    g_bpp = display_set_runtime_bpp_from_caps(&capabilities);
    LOG_INF("\tBPP: %d", g_bpp);

    LOG_INF("ocre_display_init: OK");
    lcd_initialized = 1;
}

int32_t ocre_display_init(wasm_exec_env_t exec_env)
{
    /* Ensure platform init happened and devices are ready */
    if (!display_dev || !device_is_ready(display_dev)) {
        return -1;
    }
    if (lcd_initialized == 0) {
        /* ocre_display_init() runs earlier in system bring-up. If it
        * failed to cache caps/bpp, signal NOT_READY so the app can retry.
        */
        return -1;
    }
    display_blanking_off(display_dev);
    return 0;
}

int32_t
ocre_display_get_capabilities(wasm_exec_env_t exec_env,
                              uint32_t *out_width,
                              uint32_t *out_height,
                              uint32_t *out_cpp,
                              ocre_color_mode_t *out_color_mode)
{
    if (!out_width || !out_height || !out_cpp || !out_color_mode) {
        return -1;
    }

    if (lcd_initialized == 0 || g_bpp == 0 || g_color_mode == COLOR_MODE_UNKNOWN) {
        return -1;
    }

    *out_width      = g_width;
    *out_height     = g_height;
    *out_cpp        = (uint32_t)g_bpp;
    *out_color_mode = g_color_mode;
    return 0;
}

void
ocre_display_flush(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
              int32_t y2, uint8_t *color)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    struct display_buffer_descriptor desc;

    if (g_bpp == 0 || x2 < x1 || y2 < y1)
        return;

    /* keep your current minimal validation */
    if (!wasm_runtime_validate_native_addr(module_inst, color,
                                           (uint64_t)sizeof(uint8_t)))
        return;

    uint16_t w = (uint16_t)(x2 - x1 + 1);
    uint16_t h = (uint16_t)(y2 - y1 + 1);

    desc.buf_size = (uint32_t)g_bpp * (uint32_t)w * (uint32_t)h;
    desc.width = w;
    desc.pitch = w;
    desc.height = h;

    display_write(display_dev, (uint16_t)x1, (uint16_t)y1, &desc, (void *)color);
}

void ocre_display_input_read(wasm_exec_env_t exec_env,
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

int
ocre_time_get_ms(wasm_exec_env_t exec_env)
{
    return k_uptime_get_32();
}