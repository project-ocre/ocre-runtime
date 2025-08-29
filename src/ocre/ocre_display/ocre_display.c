/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <ocre/ocre.h>
#include <ocre/api/ocre_common.h>
 #include <stdio.h>
#include <stdbool.h>
#include "ocre_display.h"
// #include "display.h"
#include "wasm_export.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#define MONITOR_HOR_RES 480
#define MONITOR_VER_RES 272
#ifndef MONITOR_ZOOM
#define MONITOR_ZOOM 1
#endif

// extern int
// ili9340_init();

static int lcd_initialized = 0;

void
display_init(void)
{
    if (lcd_initialized != 0) {
        return;
    }
    lcd_initialized = 1;
    // xpt2046_init();
    // ili9340_init();
    // display_blanking_off(NULL);
    LOG_INF("display_init");
}

void
display_flush(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
              int32_t y2, lv_color_t *color)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    struct display_buffer_descriptor desc;

    if (!wasm_runtime_validate_native_addr(module_inst, color,
                                           (uint64_t)sizeof(lv_color_t)))
        return;

    uint16_t w = x2 - x1 + 1;
    uint16_t h = y2 - y1 + 1;

    desc.buf_size = 3 * w * h;
    desc.width = w;
    desc.pitch = w;
    desc.height = h;
    // display_write(NULL, x1, y1, &desc, (void *)color);
    LOG_INF("display_flush: x1=%d, y1=%d, x2=%d, y2=%d", x1, y1, x2, y2);

    /*lv_flush_ready();*/
}

void
display_fill(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
             int32_t y2, lv_color_t *color)
{
    LOG_INF("display_fill: x1=%d, y1=%d, x2=%d, y2=%d", x1, y1, x2, y2);
}

void
display_map(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
            int32_t y2, const lv_color_t *color)
{
    LOG_INF("display_map: x1=%d, y1=%d, x2=%d, y2=%d", x1, y1, x2, y2);
}

bool
display_input_read(wasm_exec_env_t exec_env, void *data)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    lv_indev_data_t *lv_data = (lv_indev_data_t *)data;

    if (!wasm_runtime_validate_native_addr(module_inst, lv_data,
                                           (uint64_t)sizeof(lv_indev_data_t)))
        return false;

    LOG_INF("display_input_read");
    return false;

    // return touchscreen_read(lv_data);
}

void
display_deinit(wasm_exec_env_t exec_env)
{}

void
display_vdb_write(wasm_exec_env_t exec_env, void *buf, lv_coord_t buf_w,
                  lv_coord_t x, lv_coord_t y, lv_color_t *color, lv_opa_t opa)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    uint8_t *buf_xy = (uint8_t *)buf + 3 * x + 3 * y * buf_w;

    if (!wasm_runtime_validate_native_addr(module_inst, color,
                                           (uint64_t)sizeof(lv_color_t)))
        return;

    *buf_xy = color->red;
    *(buf_xy + 1) = color->green;
    *(buf_xy + 2) = color->blue;

    LOG_INF("display_vdb_write: x=%d, y=%d, opa=%d", x, y, opa);
}

int
time_get_ms(wasm_exec_env_t exec_env)
{
    return k_uptime_get_32();
}