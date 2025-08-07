/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre, a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <ocre/ocre_timers/ocre_timer.h>
#include <ocre_core_external.h>

#include <ocre/api/ocre_common.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

// Compact timer structure
typedef struct {
    uint32_t in_use: 1;
    uint32_t id: 8;        // Up to 256 timers
    uint32_t interval: 16; // Up to 65s intervals
    uint32_t periodic: 1;
    struct k_timer timer; 
    wasm_module_inst_t owner;
} ocre_timer;

#ifndef CONFIG_MAX_TIMER
#define CONFIG_MAX_TIMERS 5
#endif

// Static data
static ocre_timer timers[CONFIG_MAX_TIMERS];
static bool timer_system_initialized = false;

static void timer_callback_wrapper(struct k_timer *timer);

void ocre_timer_init(void) {
    if (timer_system_initialized) {
        LOG_INF("Timer system already initialized");
        return;
    }

    if (!common_initialized && ocre_common_init() != 0) {
        LOG_ERR("Failed to initialize common subsystem");
        return;
    }

    ocre_register_cleanup_handler(OCRE_RESOURCE_TYPE_TIMER, ocre_timer_cleanup_container);
    timer_system_initialized = true;
    LOG_INF("Timer system initialized");
}

int ocre_timer_create(wasm_exec_env_t exec_env, int id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d (max: %d)", (void *)module, id, CONFIG_MAX_TIMERS);
        return -EINVAL;
    }

    ocre_timer *timer = &timers[id - 1];
    if (timer->in_use) {
        LOG_ERR("Timer ID %d already in use", id);
        return -EBUSY;
    }

    timer->id = id;
    timer->owner = module;
    timer->in_use = 1;
    k_timer_init(&timer->timer, timer_callback_wrapper, NULL);
    ocre_increment_resource_count(module, OCRE_RESOURCE_TYPE_TIMER);
    LOG_INF("Created timer %d for module %p", id, (void *)module);
    return 0;
}

int ocre_timer_delete(wasm_exec_env_t exec_env, ocre_timer_t id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }
    k_timer_stop(&timer->timer);
    timer->in_use = 0;
    timer->owner = NULL;
    ocre_decrement_resource_count(module, OCRE_RESOURCE_TYPE_TIMER);
    LOG_INF("Deleted timer %d", id);
    return 0;
}

int ocre_timer_start(wasm_exec_env_t exec_env, ocre_timer_t id, int interval, int is_periodic) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }

    if (interval <= 0 || interval > 65535) {
        LOG_ERR("Invalid interval %dms (must be 1-65535ms)", interval);
        return -EINVAL;
    }

    timer->interval = interval;
    timer->periodic = is_periodic;
    k_timeout_t duration = K_MSEC(interval);
    k_timeout_t period = is_periodic ? duration : K_NO_WAIT;
    k_timer_start(&timer->timer, duration, period);
    LOG_INF("Started timer %d with interval %dms, periodic=%d", id, interval, is_periodic);
    return 0;
}

int ocre_timer_stop(wasm_exec_env_t exec_env, ocre_timer_t id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }
    k_timer_stop(&timer->timer);
    LOG_INF("Stopped timer %d", id);
    return 0;
}

int ocre_timer_get_remaining(wasm_exec_env_t exec_env, ocre_timer_t id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }
    int remaining = k_ticks_to_ms_floor32(k_timer_remaining_ticks(&timer->timer));
    LOG_INF("Timer %d remaining time: %dms", id, remaining);
    return remaining;
}

void ocre_timer_cleanup_container(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Invalid module instance for cleanup");
        return;
    }

    for (int i = 0; i < CONFIG_MAX_TIMERS; i++) {
        if (timers[i].in_use && timers[i].owner == module_inst) {
            k_timer_stop(&timers[i].timer);
            timers[i].in_use = 0;
            timers[i].owner = NULL;
            ocre_decrement_resource_count(module_inst, OCRE_RESOURCE_TYPE_TIMER);
            LOG_INF("Cleaned up timer %d for module %p", i + 1, (void *)module_inst);
        }
    }
    LOG_INF("Cleaned up timer resources for module %p", (void *)module_inst);
}

static void timer_callback_wrapper(struct k_timer *timer) {
    if (!timer_system_initialized || !common_initialized || !ocre_event_queue_initialized) {
        LOG_ERR("Timer, common, or event queue not initialized, skipping callback");
        return;
    }
    if (!timer) {
        LOG_ERR("Null timer pointer in callback");
        return;
    }
    if ((uintptr_t)ocre_event_queue_buffer_ptr % 4 != 0) {
        LOG_ERR("ocre_event_queue_buffer misaligned: %p", (void *)ocre_event_queue_buffer_ptr);
        return;
    }
    LOG_DBG("Timer callback for timer %p", (void *)timer);
    LOG_DBG("ocre_event_queue at %p, buffer at %p", (void *)&ocre_event_queue, (void *)ocre_event_queue_buffer_ptr);
    for (int i = 0; i < CONFIG_MAX_TIMERS; i++) {
        if (timers[i].in_use && &timers[i].timer == timer && timers[i].owner) {
            ocre_event_t event;
            event.type = OCRE_RESOURCE_TYPE_TIMER;
            event.data.timer_event.timer_id = timers[i].id;
            event.owner = timers[i].owner;

            LOG_DBG("Creating timer event: type=%d, id=%d, for owner %p", event.type, event.data.timer_event.timer_id,
                    (void *)timers[i].owner);
            LOG_DBG("Event address: %p, Queue buffer: %p", (void *)&event, (void *)ocre_event_queue_buffer_ptr);
            k_spinlock_key_t key = k_spin_lock(&ocre_event_queue_lock);
            if (k_msgq_put(&ocre_event_queue, &event, K_NO_WAIT) != 0) {
                LOG_ERR("Failed to queue timer event for timer %d", timers[i].id);
            } else {
                LOG_DBG("Queued timer event for timer %d", timers[i].id);
            }
            k_spin_unlock(&ocre_event_queue_lock, key);
        }
    }
}
