/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_TIMER_H
#define OCRE_TIMER_H

#include <errno.h>
#include <stdbool.h>

#include <zephyr/kernel.h>

typedef struct k_timer_ocre {
    /*Here can be added all the other variables needed for timers*/
    /*Zephyr kernel timer */
    struct k_timer timer;
} k_timer_ocre;

typedef void *ocre_timer_t;

typedef void (*ocre_timer_callback_t)(ocre_timer_t timer);

/**
 * Creates a timer.
 * 
 * @param callback Funcation callback to run when time expires
 * 
 * @return Handle to the timer
 */
ocre_timer_t ocre_timer_create(ocre_timer_callback_t callback);

/**
 * Deletes a timer
 * 
 * @param id Handle to the timer to be deleted.
 * 
 * @return 0 on success, -1 on failure with errno set.
 */
int ocre_timer_delete(ocre_timer_t id);

/**
 * Starts a timer.
 * 
 * @param id Handle to the timer to be started.
 * @param interval Time in milliseconds after which the timer expires.
 * @param is_periodic If true, the timer will restart automatically after expiring.
 * 
 * @return 0 on success, -1 on failure with errno set.
 */
int ocre_timer_start(ocre_timer_t id, int interval, bool is_periodic);

/**
 * Stops a timer.
 * 
 * @param id Handle to the timer to be stopped.
 * 
 * @return 0 on success, -1 on failure with errno set.
 */
int ocre_timer_stop(ocre_timer_t id);

/**
 * This routine computes the time remaining before a running timer next expires,
 * in units of system ticks. If the timer is not running, it returns zero.
 * 
 * @param id Handle to the timer which status we want to get
 * 
 * @return 0 if the timer is not running, otherwise the remaining time in milliseconds
 */
int ocre_timer_get_remaining(ocre_timer_t id);

#endif