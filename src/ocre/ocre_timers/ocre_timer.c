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

// Timer dispatcher function for WASM callbacks
static wasm_function_inst_t timer_dispatcher_func = NULL;
static bool timer_system_initialized = false;
static wasm_module_inst_t current_module_inst = NULL; // Store current module instance
static bool module_initialization_complete = false;

// Maximum number of timers that can be active simultaneously
#define MAX_TIMERS 5

// Timer state structure
typedef struct {
    struct k_timer timer;
    bool in_use;
    uint32_t id;
    wasm_exec_env_t exec_env; // Store execution environment
} k_timer_ocre;

static k_timer_ocre timers[MAX_TIMERS] = {0};

// Set the current module instance
void ocre_timer_set_module_inst(wasm_module_inst_t module_inst) {
    current_module_inst = module_inst;
}

void ocre_timer_module_init_complete(void) {
    module_initialization_complete = true;
    LOG_INF("Module initialization completed\n");
}

static void wasm_timer_callback(struct k_timer *timer) {
    if (!timer_dispatcher_func || !module_initialization_complete) {
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

// Find first available timer slot
static int find_free_timer(void) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timers[i].in_use) {
            return i;
        }
    }
    return -1;
}

// Get timer structure from ID
static k_timer_ocre *get_timer_from_id(ocre_timer_t id) {
    if (id == 0 || id > MAX_TIMERS) {
        return NULL;
    }

    int index = id - 1;
    if (!timers[index].in_use) {
        return NULL;
    }

    return &timers[index];
}

void ocre_timer_init(void) {
    if (!timer_system_initialized) {
        for (int i = 0; i < MAX_TIMERS; i++) {
            timers[i].in_use = false;
            timers[i].id = 0;
            timers[i].exec_env = NULL;
        }
        timer_system_initialized = true;
        module_initialization_complete = false;
        LOG_INF("Timer system initialized\n");
    }
}
int ocre_timer_create(int id) {
    if (!module_initialization_complete) {
        LOG_ERR("Ignoring timer creation during module initialization\n");
        return 0; // Return success to avoid initialization errors
    }

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

    int index = id - 1;
    if (timers[index].in_use) {
        LOG_ERR("Timer ID %d is already in use\n", id);
        errno = EEXIST;
        return -1;
    }

    timers[index].exec_env = wasm_runtime_get_exec_env_singleton(current_module_inst);
    k_timer_init(&timers[index].timer, wasm_timer_callback, NULL);
    timers[index].in_use = true;
    timers[index].id = id;

    LOG_INF("Timer created successfully: ID %d\n", id);
    return 0;
}

int ocre_timer_delete(ocre_timer_t id) {
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer) {
        errno = EINVAL;
        return -1;
    }

    k_timer_stop(&timer->timer);
    timer->in_use = false;
    return 0;
}

int ocre_timer_start(ocre_timer_t id, int interval, int is_periodic) {
    LOG_INF("Timer start called for ID: %d\n", id);
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer) {
        errno = EINVAL;
        return -1;
    }

    // Convert milliseconds to ticks
    k_timeout_t start_timeout = K_MSEC(interval);
    k_timeout_t repeat_timeout = is_periodic ? K_MSEC(interval) : K_NO_WAIT;

    k_timer_start(&timer->timer, start_timeout, repeat_timeout);
    return 0;
}

int ocre_timer_stop(ocre_timer_t id) {
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer) {
        errno = EINVAL;
        return -1;
    }

    k_timer_stop(&timer->timer);
    return 0;
}

int ocre_timer_get_remaining(ocre_timer_t id) {
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer) {
        errno = EINVAL;
        return -1;
    }

    // Convert ticks to milliseconds
    return k_timer_remaining_get(&timer->timer);
}

// Function to set the WASM dispatcher function
void ocre_timer_set_dispatcher(wasm_function_inst_t func) {
    timer_dispatcher_func = func;
    LOG_INF("Timer dispatcher function set\n");
}