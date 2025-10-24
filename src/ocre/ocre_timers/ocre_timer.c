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
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#ifdef CONFIG_ZEPHYR
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);
#else
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#endif

/* Platform-specific timer structures */
#ifdef CONFIG_ZEPHYR
// Compact timer structure for Zephyr
typedef struct {
    uint32_t in_use: 1;
    uint32_t id: 8;        // Up to 256 timers
    uint32_t interval: 16; // Up to 65s intervals
    uint32_t periodic: 1;
    struct k_timer timer; 
    wasm_module_inst_t owner;
} ocre_timer;

#else /* POSIX */
// POSIX timer structure
typedef struct {
    uint32_t in_use: 1;
    uint32_t id: 8;        // Up to 256 timers
    uint32_t interval: 16; // Up to 65s intervals
    uint32_t periodic: 1;
    timer_t timer;
    pthread_t thread;
    bool thread_running;
    wasm_module_inst_t owner;
} ocre_timer;

/* POSIX timer thread function */
static void* posix_timer_thread(void* arg);
#endif

#ifndef CONFIG_MAX_TIMER
#define CONFIG_MAX_TIMERS 5
#endif

// Static data
static ocre_timer timers[CONFIG_MAX_TIMERS];
static bool timer_system_initialized = false;

#ifdef CONFIG_ZEPHYR
static void timer_callback_wrapper(struct k_timer *timer);
#else
static void posix_timer_signal_handler(int sig, siginfo_t *si, void *uc);
static void posix_send_timer_event(int timer_id);
#endif

void ocre_timer_init(void) {
    if (timer_system_initialized) {
        LOG_INF("Timer system already initialized");
        return;
    }

    if (!common_initialized && ocre_common_init() != 0) {
        LOG_ERR("Failed to initialize common subsystem");
        return;
    }

#ifndef CONFIG_ZEPHYR
    // Setup POSIX signal handler for timer callbacks
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = posix_timer_signal_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        LOG_ERR("Failed to setup POSIX timer signal handler");
        return;
    }
#endif

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
    
#ifdef CONFIG_ZEPHYR
    k_timer_init(&timer->timer, timer_callback_wrapper, NULL);
#else
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_int = id;
    
    if (timer_create(CLOCK_REALTIME, &sev, &timer->timer) == -1) {
        LOG_ERR("Failed to create POSIX timer %d", id);
        timer->in_use = 0;
        return -EINVAL;
    }
    timer->thread_running = false;
#endif
    
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
    
#ifdef CONFIG_ZEPHYR
    k_timer_stop(&timer->timer);
#else
    timer_delete(&timer->timer);
    if (timer->thread_running) {
        pthread_cancel(timer->thread);
        timer->thread_running = false;
    }
#endif
    
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
    
#ifdef CONFIG_ZEPHYR
    k_timeout_t duration = K_MSEC(interval);
    k_timeout_t period = is_periodic ? duration : K_NO_WAIT;
    k_timer_start(&timer->timer, duration, period);
#else
    struct itimerspec its;
    its.it_value.tv_sec = interval / 1000;
    its.it_value.tv_nsec = (interval % 1000) * 1000000;
    
    if (is_periodic) {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    } else {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
    }
    
    if (timer_settime(timer->timer, 0, &its, NULL) == -1) {
        LOG_ERR("Failed to start POSIX timer %d", id);
        return -EINVAL;
    }
#endif
    
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
    
#ifdef CONFIG_ZEPHYR
    k_timer_stop(&timer->timer);
#else
    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    timer_settime(timer->timer, 0, &its, NULL);
#endif
    
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
    
    int remaining;
#ifdef CONFIG_ZEPHYR
    remaining = k_ticks_to_ms_floor32(k_timer_remaining_ticks(&timer->timer));
#else
    struct itimerspec its;
    if (timer_gettime(timer->timer, &its) == -1) {
        LOG_ERR("Failed to get remaining time for timer %d", id);
        return -EINVAL;
    }
    remaining = its.it_value.tv_sec * 1000 + its.it_value.tv_nsec / 1000000;
#endif
    
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
#ifdef CONFIG_ZEPHYR
            k_timer_stop(&timers[i].timer);
#else
            timer_delete(&timers[i].timer);
            if (timers[i].thread_running) {
                pthread_cancel(timers[i].thread);
                timers[i].thread_running = false;
            }
#endif
            timers[i].in_use = 0;
            timers[i].owner = NULL;
            ocre_decrement_resource_count(module_inst, OCRE_RESOURCE_TYPE_TIMER);
            LOG_INF("Cleaned up timer %d for module %p", i + 1, (void *)module_inst);
        }
    }
    LOG_INF("Cleaned up timer resources for module %p", (void *)module_inst);
}

#ifdef CONFIG_ZEPHYR
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

#else /* POSIX */

static void posix_timer_signal_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig;
    (void)uc;
    if (si && si->si_value.sival_int > 0 && si->si_value.sival_int <= CONFIG_MAX_TIMERS) {
        posix_send_timer_event(si->si_value.sival_int);
    }
}

static void posix_send_timer_event(int timer_id) {
    if (!timer_system_initialized || !common_initialized || !ocre_event_queue_initialized) {
        LOG_ERR("Timer, common, or event queue not initialized, skipping callback");
        return;
    }
    
    int index = timer_id - 1;
    if (index < 0 || index >= CONFIG_MAX_TIMERS || !timers[index].in_use) {
        LOG_ERR("Invalid timer ID %d in callback", timer_id);
        return;
    }
    
    ocre_event_t event;
    event.type = OCRE_RESOURCE_TYPE_TIMER;
    event.data.timer_event.timer_id = timer_id;
    event.owner = timers[index].owner;
    
    LOG_DBG("Creating timer event: type=%d, id=%d, for owner %p", event.type, timer_id, (void *)timers[index].owner);
    
    // Use the POSIX message queue implementation
    extern posix_msgq_t ocre_event_queue;
    extern posix_spinlock_t ocre_event_queue_lock;
    
    posix_spinlock_key_t key = OCRE_SPINLOCK_LOCK(&ocre_event_queue_lock);
    if (posix_msgq_put(&ocre_event_queue, &event) != 0) {
        LOG_ERR("Failed to queue timer event for timer %d", timer_id);
    } else {
        LOG_DBG("Queued timer event for timer %d", timer_id);
    }
    OCRE_SPINLOCK_UNLOCK(&ocre_event_queue_lock, key);
}

#endif
