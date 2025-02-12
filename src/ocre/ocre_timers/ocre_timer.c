/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ocre/ocre.h>
#include "ocre_timer.h"
#include "wasm_export.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_TIMERS 5

typedef struct {
    struct k_timer timer;
    bool in_use;
    uint32_t id;
    wasm_exec_env_t exec_env;
} k_timer_ocre;

static k_timer_ocre timers[MAX_TIMERS] = {0};
static wasm_function_inst_t timer_dispatcher_func = NULL;
static bool timer_system_initialized = false;
static wasm_module_inst_t current_module_inst = NULL;

static void wasm_timer_callback(struct k_timer *timer) {
    if (!timer_dispatcher_func || !current_module_inst) {
        return;
    }

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (&timers[i].timer == timer && timers[i].in_use) {
            uint32_t args[1] = {timers[i].id};
            if (timers[i].exec_env) {
                wasm_runtime_call_wasm(timers[i].exec_env, timer_dispatcher_func, 1, args);
            }
            break;
        }
    }
}

void ocre_timer_set_module_inst(wasm_module_inst_t module_inst) {
    current_module_inst = module_inst;
}

static k_timer_ocre *get_timer_from_id(ocre_timer_t id) {
    if (id == 0 || id > MAX_TIMERS) {
        return NULL;
    }

    int index = id - 1;
    return &timers[index]; // Let the caller check in_use
}

void ocre_timer_init(wasm_exec_env_t exec_env) {
    if (!timer_system_initialized) {
        for (int i = 0; i < MAX_TIMERS; i++) {
            timers[i].in_use = false;
            timers[i].id = 0;
            timers[i].exec_env = NULL; // Initialize exec_env
        }
        timer_system_initialized = true;
        LOG_INF("Timer system initialized\n");
    }
}

int ocre_timer_create(wasm_exec_env_t exec_env, int id) {
    if (!timer_system_initialized || !current_module_inst) {
        LOG_ERR("ERROR: Timer system not properly initialized\n");
        errno = EINVAL;
        return -1;
    }

    LOG_DBG("Timer create called for ID: %d\n", id);

    if (id <= 0 || id > MAX_TIMERS) {
        LOG_ERR("Invalid timer ID: %d (Expected between 1-%d)\n", id, MAX_TIMERS);
        errno = EINVAL;
        return -1;
    }

    k_timer_ocre *timer = get_timer_from_id(id);
    if (timer->in_use) {
        LOG_ERR("Timer ID %d is already in use\n", id);
        errno = EEXIST;
        return -1;
    }

    timer->exec_env = wasm_runtime_get_exec_env_singleton(current_module_inst);
    k_timer_init(&timer->timer, wasm_timer_callback, NULL);
    timer->in_use = true;
    timer->id = id;

    LOG_INF("Timer created successfully: ID %d\n", id);
    return 0;
}

int ocre_timer_delete(wasm_exec_env_t exec_env, ocre_timer_t id) {
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer || !timer->in_use) { // Fixed condition
        LOG_ERR("ERROR: timer %d not found or not in use\n", id);
        errno = EINVAL;
        return -1;
    }

    k_timer_stop(&timer->timer);
    timer->in_use = false;
    timer->exec_env = NULL; // Clear exec_env
    return 0;
}

int ocre_timer_start(wasm_exec_env_t exec_env, ocre_timer_t id, int interval, int is_periodic) {
    LOG_INF("Timer start called for ID: %d\n", id);
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer || !timer->in_use) { // Fixed condition
        LOG_ERR("ERROR: timer %d not found or not in use\n", id);
        errno = EINVAL;
        return -1;
    }

    if (interval <= 0) { // Added interval validation
        LOG_ERR("Invalid interval: %d\n", interval);
        errno = EINVAL;
        return -1;
    }

    // Convert milliseconds to ticks for more accurate timing
    k_timeout_t start_timeout = K_MSEC(interval);
    k_timeout_t repeat_timeout = is_periodic ? K_MSEC(interval) : K_NO_WAIT;

    k_timer_start(&timer->timer, start_timeout, repeat_timeout);
    return 0;
}

int ocre_timer_stop(wasm_exec_env_t exec_env, ocre_timer_t id) {
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer || !timer->in_use) { // Fixed condition
        LOG_ERR("ERROR: timer %d not found or not in use\n", id);
        errno = EINVAL;
        return -1;
    }

    k_timer_stop(&timer->timer);
    return 0;
}

int ocre_timer_get_remaining(wasm_exec_env_t exec_env, ocre_timer_t id) {
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer || !timer->in_use) { // Fixed condition
        LOG_ERR("ERROR: timer %d not found or not in use\n", id);
        errno = EINVAL;
        return -1;
    }

    // Convert ticks to milliseconds for accurate remaining time
    k_ticks_t remaining_ticks = k_timer_remaining_ticks(&timer->timer);
    uint32_t remaining_ms = k_ticks_to_ms_floor64(remaining_ticks);

    return remaining_ms;
}

void ocre_timer_set_dispatcher(wasm_exec_env_t exec_env, wasm_function_inst_t func) {
    timer_dispatcher_func = func;
    LOG_INF("Timer dispatcher function set\n");
}