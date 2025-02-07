/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_timer.h"
#include <stdlib.h>

ocre_timer_t ocre_timer_create(void) {
    struct k_timer_ocre *timer = malloc(sizeof(struct k_timer_ocre));

    if (!timer) {
        errno = ENOMEM; // Out of memory
        return NULL;
    }

    k_timer_init(&timer->timer, (void (*)(struct k_timer *))callback, NULL);

    return (ocre_timer_t)timer;
}

int ocre_timer_delete(ocre_timer_t id) {
    if (!id) {
        errno = EINVAL; // Invalid argument
        return -1;
    }

    free(id);

    return 0;
}

int ocre_timer_start(ocre_timer_t id, int interval, bool is_periodic) {
    if (!id) {
        errno = EINVAL; // Invalid argument
        return -1;
    }

    struct k_timer_ocre *timer = (struct k_timer_ocre *)id;
    k_timeout_t start_timeout = Z_TIMEOUT_MS(interval);
    k_timeout_t repeat_timeout = is_periodic ? Z_TIMEOUT_MS(interval) : Z_TIMEOUT_MS(0);

    k_timer_start(&timer->timer, start_timeout, repeat_timeout);

    return 0;
}

int ocre_timer_stop(ocre_timer_t id) {
    if (!id) {
        errno = EINVAL; // Invalid argument
        return -1;
    }

    struct k_timer_ocre *timer = (struct k_timer_ocre *)id;
    k_timer_stop(&timer->timer);

    return 0;
}

int ocre_timer_get_remaining(ocre_timer_t id) {
    struct k_timer_ocre *timer = (struct k_timer_ocre *)id;

    return k_timer_remaining_get((&timer->timer));
}
