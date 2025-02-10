/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_timer.h"
#include "wasm_export.h"
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

// Timer dispatcher function for WASM callbacks
wasm_function_inst_t timer_dispatcher_func = NULL;

// Maximum number of timers that can be active simultaneously
#define MAX_TIMERS 5

// Timer state structure
typedef struct {
    struct k_timer timer;
    bool in_use;
    uint32_t id;
} k_timer_ocre;

// Global timer array
static k_timer_ocre timers[MAX_TIMERS] = {0};

// Timer callback function that dispatches to WASM
static void wasm_timer_callback(struct k_timer *timer) {
    if (timer_dispatcher_func) {
        // Find the timer ID from the timer pointer
        for (int i = 0; i < MAX_TIMERS; i++) {
            if (&timers[i].timer == timer) {
                uint32_t args[1] = {timers[i].id};
                wasm_runtime_call_wasm(NULL, timer_dispatcher_func, 1, args);
                break;
            }
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

int ocre_timer_create(ocre_timer_t id) {
    int index = id - 1;
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].in_use == false) {
            int index = i;
            break;
        }
    }
    if (id <= 0 || id > MAX_TIMERS) {
        printf("Invalid timer ID: %d\n", id);
        errno = EINVAL;
        return -1;
    }

    if (timers[index].in_use) {
        printf("Timer %d already in use\n", id);
        errno = EEXIST;
        return -1;
    }

    printf("Creating timer with ID %d\n", id);
    k_timer_init(&timers[index].timer, wasm_timer_callback, NULL);
    timers[index].in_use = true;
    timers[index].id = id;

    return id;
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
}