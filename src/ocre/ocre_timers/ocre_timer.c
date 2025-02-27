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

#define TIMER_STACK_SIZE      2048
#define TIMER_THREAD_PRIORITY 5
#define WASM_STACK_SIZE       (8 * 1024)

typedef struct {
    struct k_timer timer;
    bool in_use;
    uint32_t id;
} k_timer_ocre;

static K_THREAD_STACK_DEFINE(timer_thread_stack, TIMER_STACK_SIZE);
static struct k_thread timer_thread;
static k_tid_t timer_thread_id;

K_MSGQ_DEFINE(timer_msgq, sizeof(uint32_t), 10, 4);

// Timer management
static k_timer_ocre timers[CONFIG_MAX_TIMERS] = {0};
static wasm_function_inst_t timer_dispatcher_func = NULL;
static bool timer_system_initialized = false;
static wasm_module_inst_t current_module_inst = NULL;
static wasm_exec_env_t shared_exec_env = NULL;

void ocre_timer_cleanup_container(wasm_module_inst_t module_inst) {
    if (!timer_system_initialized || !module_inst) {
        LOG_ERR("Timer system not properly initialized");
        return;
    }

    // Only clean up timers if they belong to the specified module instance
    if (module_inst != current_module_inst) {
        LOG_WRN("Cleanup requested for non-active module instance");
        return;
    }

    int cleaned_count = 0;
    for (int i = 0; i < CONFIG_MAX_TIMERS; i++) {
        if (timers[i].in_use) {
            k_timer_stop(&timers[i].timer);
            timers[i].in_use = false;
            timers[i].id = 0;
            cleaned_count++;
        }
    }

    LOG_INF("Cleaned up %d timers for container", cleaned_count);
}

// Thread function to process timer callbacks
static void timer_thread_fn(void *arg1, void *arg2, void *arg3) {
    uint32_t timer_id;

    while (1) {
        // Wait for a timer message
        if (k_msgq_get(&timer_msgq, &timer_id, K_FOREVER) == 0) {
            if (!timer_dispatcher_func || !current_module_inst || !shared_exec_env) {
                LOG_ERR("Timer system not properly initialized");
                continue;
            }

            LOG_DBG("Processing timer ID: %d", timer_id);
            uint32_t args[1] = {timer_id};

            // Execute the WASM callback
            bool call_success = wasm_runtime_call_wasm(shared_exec_env, timer_dispatcher_func, 1, args);

            if (!call_success) {
                const char *error = wasm_runtime_get_exception(current_module_inst);
                LOG_ERR("Failed to call WASM function: %s", error ? error : "Unknown error");
            } else {
                LOG_INF("Successfully called WASM function for timer %d", timer_id);
            }
        }
    }
}

static void wasm_timer_callback(struct k_timer *timer) {
    for (int i = 0; i < CONFIG_MAX_TIMERS; i++) {
        if (&timers[i].timer == timer && timers[i].in_use) {
            // Send timer ID to the processing thread
            if (k_msgq_put(&timer_msgq, &timers[i].id, K_NO_WAIT) != 0) {
                LOG_ERR("Failed to queue timer callback for ID: %d", timers[i].id);
            }
            break;
        }
    }
}

void ocre_timer_set_module_inst(wasm_module_inst_t module_inst) {
    current_module_inst = module_inst;
    if (shared_exec_env) {
        wasm_runtime_destroy_exec_env(shared_exec_env);
    }
    shared_exec_env = wasm_runtime_create_exec_env(module_inst, WASM_STACK_SIZE);
}

static k_timer_ocre *get_timer_from_id(ocre_timer_t id) {
    if (id == 0 || id > CONFIG_MAX_TIMERS) {
        return NULL;
    }
    return &timers[id - 1];
}

void ocre_timer_init(void) {
    if (!timer_system_initialized) {
        // Initialize timer array
        for (int i = 0; i < CONFIG_MAX_TIMERS; i++) {
            timers[i].in_use = false;
            timers[i].id = 0;
        }

        // Create the timer processing thread
        timer_thread_id = k_thread_create(&timer_thread, timer_thread_stack, K_THREAD_STACK_SIZEOF(timer_thread_stack),
                                          timer_thread_fn, NULL, NULL, NULL, TIMER_THREAD_PRIORITY, 0, K_NO_WAIT);

        if (timer_thread_id == NULL) {
            LOG_ERR("Failed to create timer thread");
            return;
        }

        k_thread_name_set(timer_thread_id, "timer_thread");
        timer_system_initialized = true;
        LOG_INF("Timer system initialized with dedicated thread");
    }
}

int ocre_timer_create(wasm_exec_env_t exec_env, int id) {
    if (!timer_system_initialized || !current_module_inst) {
        LOG_ERR("Timer system not properly initialized");
        errno = EINVAL;
        return -1;
    }

    if (id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid timer ID: %d (Expected between 1-%d)", id, CONFIG_MAX_TIMERS);
        errno = EINVAL;
        return -1;
    }

    k_timer_ocre *timer = get_timer_from_id(id);
    if (timer->in_use) {
        LOG_ERR("Timer ID %d is already in use", id);
        errno = EEXIST;
        return -1;
    }

    ocre_timer_set_dispatcher(exec_env);
    k_timer_init(&timer->timer, wasm_timer_callback, NULL);
    timer->in_use = true;
    timer->id = id;

    LOG_INF("Timer created successfully: ID %d", id);
    return 0;
}

int ocre_timer_delete(wasm_exec_env_t exec_env, ocre_timer_t id) {
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer || !timer->in_use) {
        LOG_ERR("ERROR: timer %d not found or not in use\n", id);
        errno = EINVAL;
        return -1;
    }

    k_timer_stop(&timer->timer);
    timer->in_use = false;
    return 0;
}

int ocre_timer_start(wasm_exec_env_t exec_env, ocre_timer_t id, int interval, int is_periodic) {
    LOG_INF("Timer start called for ID: %d\n", id);
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer || !timer->in_use) {
        LOG_ERR("ERROR: timer %d not found or not in use\n", id);
        errno = EINVAL;
        return -1;
    }

    if (interval <= 0) {
        LOG_ERR("Invalid interval: %d\n", interval);
        errno = EINVAL;
        return -1;
    }

    k_timeout_t start_timeout = K_MSEC(interval);
    k_timeout_t repeat_timeout = is_periodic ? K_MSEC(interval) : K_NO_WAIT;

    k_timer_start(&timer->timer, start_timeout, repeat_timeout);
    return 0;
}

int ocre_timer_stop(wasm_exec_env_t exec_env, ocre_timer_t id) {
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer || !timer->in_use) {
        LOG_ERR("ERROR: timer %d not found or not in use\n", id);
        errno = EINVAL;
        return -1;
    }

    k_timer_stop(&timer->timer);
    return 0;
}

int ocre_timer_get_remaining(wasm_exec_env_t exec_env, ocre_timer_t id) {
    k_timer_ocre *timer = get_timer_from_id(id);
    if (!timer) {
        LOG_ERR("ERROR: Timer ID %d not found", id);
        errno = EINVAL;
        return -1;
    }
    if (!timer->in_use) {
        LOG_ERR("ERROR: Timer ID %d is not in use", id);
        errno = EINVAL;
        return -1;
    }
    k_ticks_t remaining_ticks = k_timer_remaining_ticks(&timer->timer);
    uint32_t remaining_ms = k_ticks_to_ms_floor64(remaining_ticks);

    return (int)remaining_ms;
}
void ocre_timer_set_dispatcher(wasm_exec_env_t exec_env) {
    if (!current_module_inst) {
        LOG_ERR("No active WASM module instance");
        return;
    }

    wasm_function_inst_t func = wasm_runtime_lookup_function(current_module_inst, "timer_callback");
    if (!func) {
        LOG_ERR("Failed to find 'timer_callback' in WASM module");
        return;
    }

    timer_dispatcher_func = func;
    LOG_INF("WASM timer dispatcher function set successfully");
}