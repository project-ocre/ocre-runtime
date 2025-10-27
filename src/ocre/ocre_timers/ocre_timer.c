/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre, a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <ocre_core_external.h>
#include <ocre/ocre_timers/ocre_timer.h>

#include <ocre/api/ocre_common.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

/* Unified timer structure using core_timer API */
typedef struct {
    uint32_t in_use: 1;
    uint32_t id: 8;        // Up to 256 timers
    uint32_t interval: 16; // Up to 65s intervals
    uint32_t periodic: 1;
    uint32_t running: 1;   // Track if timer is currently running
    uint32_t start_time;   // Start time for remaining time calculations
    core_timer_t timer;    // Unified core timer
    wasm_module_inst_t owner;
} ocre_timer_internal;

#ifndef CONFIG_MAX_TIMER
#define CONFIG_MAX_TIMERS 5
#endif

// Static data
static ocre_timer_internal timers[CONFIG_MAX_TIMERS];
static bool timer_system_initialized = false;

static void unified_timer_callback(void *user_data);

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

    ocre_timer_internal *timer = &timers[id - 1];
    if (timer->in_use) {
        LOG_ERR("Timer ID %d already in use", id);
        return -EBUSY;
    }

    timer->id = id;
    timer->owner = module;
    timer->in_use = 1;
    
    // Initialize unified core timer
    if (core_timer_init(&timer->timer, unified_timer_callback, timer) != 0) {
        LOG_ERR("Failed to initialize core timer %d", id);
        timer->in_use = 0;
        return -EINVAL;
    }
    
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

    ocre_timer_internal *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }
    
    // Stop unified core timer
    core_timer_stop(&timer->timer);
    
    timer->in_use = 0;
    timer->running = 0;
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

    ocre_timer_internal *timer = &timers[id - 1];
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
    timer->start_time = core_uptime_get();
    timer->running = 1;
    
    // Start unified core timer
    int period_ms = is_periodic ? interval : 0;
    if (core_timer_start(&timer->timer, interval, period_ms) != 0) {
        LOG_ERR("Failed to start core timer %d", id);
        timer->running = 0;
        return -EINVAL;
    }
    
    LOG_INF("Started timer %d with interval %dms, periodic=%d", id, interval, is_periodic);
    return 0;
}

int ocre_timer_stop(wasm_exec_env_t exec_env, ocre_timer_t id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer_internal *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }
    
    // Stop unified core timer
    core_timer_stop(&timer->timer);
    timer->running = 0;
    
    LOG_INF("Stopped timer %d", id);
    return 0;
}

int ocre_timer_get_remaining(wasm_exec_env_t exec_env, ocre_timer_t id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer_internal *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }
    
    int remaining;
    if (!timer->running) {
        remaining = 0;
    } else {
        uint32_t current_time = core_uptime_get();
        uint32_t elapsed = current_time - timer->start_time;
        if (elapsed >= timer->interval) {
            remaining = 0;  // Timer should have expired
        } else {
            remaining = timer->interval - elapsed;
        }
    }
    
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
            // Stop unified core timer
            core_timer_stop(&timers[i].timer);
            timers[i].in_use = 0;
            timers[i].running = 0;
            timers[i].owner = NULL;
            ocre_decrement_resource_count(module_inst, OCRE_RESOURCE_TYPE_TIMER);
            LOG_DBG("Cleaned up timer %d for module %p", i + 1, (void *)module_inst);
        }
    }
    LOG_DBG("Cleaned up timer resources for module %p", (void *)module_inst);
}

/* Unified timer callback using core_timer API */
static void unified_timer_callback(void *user_data) {
    if (!timer_system_initialized || !common_initialized || !ocre_event_queue_initialized) {
        LOG_ERR("Timer, common, or event queue not initialized, skipping callback");
        return;
    }
    
    ocre_timer_internal *timer = (ocre_timer_internal *)user_data;
    if (!timer || !timer->in_use || !timer->owner) {
        LOG_ERR("Invalid timer in callback: %p", (void *)timer);
        return;
    }
    
    LOG_DBG("Timer callback for timer %d", timer->id);
    
    // For non-periodic timers, mark as not running
    if (!timer->periodic) {
        timer->running = 0;
    } else {
        // For periodic timers, update start time for next cycle
        timer->start_time = core_uptime_get();
    }
    
    // Create and queue timer event
    ocre_event_t event;
    event.type = OCRE_RESOURCE_TYPE_TIMER;
    event.data.timer_event.timer_id = timer->id;
    event.owner = timer->owner;
    
    LOG_DBG("Creating timer event: type=%d, id=%d, for owner %p", event.type, timer->id, (void *)timer->owner);
    
    core_spinlock_key_t key = core_spinlock_lock(&ocre_event_queue_lock);
    if (core_eventq_put(&ocre_event_queue, &event) != 0) {
        LOG_ERR("Failed to queue timer event for timer %d", timer->id);
    } else {
        LOG_DBG("Queued timer event for timer %d", timer->id);
    }
    core_spinlock_unlock(&ocre_event_queue_lock, key);
}
