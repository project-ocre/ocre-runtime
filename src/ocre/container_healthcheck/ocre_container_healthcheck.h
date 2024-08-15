/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_WATCHDOG_H
#define OCRE_WATCHDOG_H

#include <zephyr/kernel.h>

/**
 * Structure to hold the OCRE health check parameters.
 */
typedef struct ocre_healthcheck {
    uint32_t interval;
    uint32_t timeout;
    bool enabled;
    uint32_t is_alive_cnt;
    uint32_t is_alive_cnt_last;
    struct k_timer timer;
} ocre_healthcheck;

/**
 * Initializes the OCRE health check structure.
 * 
 * @param WDT Pointer to the OCRE health check structure.
 * @param timeout Timeout value for the watchdog timer in milliseconds.
 * 
 * @return 0 on success, -1 on error (e.g., null pointer).
 */
int ocre_healthcheck_init(ocre_healthcheck *WDT, int timeout);

/** 
 * Reset/Trigger the OCRE health check structure.
 * 
 * @param WDT Pointer to the OCRE health check structure.
 * 
 * @return 0 on success, -1 on error (e.g., null pointer).
 */
int ocre_healthcheck_reinit(ocre_healthcheck *WDT);

/** 
 * Starts the OCRE health check timer.
 * 
 * @param WDT Pointer to the OCRE health check structure.
 * 
 * @return 0 on success, -1 on error (e.g., null pointer).
 */
int ocre_healthcheck_start(ocre_healthcheck *WDT);

/** 
 * Stops the OCRE health check timer.
 * 
 * @param WDT Pointer to the OCRE health check structure.
 * 
 * @return 0 on success, -1 on error (e.g., null pointer).
 */
int ocre_healthcheck_stop(ocre_healthcheck *WDT);

/**
 * Gets the remaining time for the OCRE health check timer.
 * 
 * @param WDT Pointer to the OCRE health check structure.
 * 
 * @return Remaining time in milliseconds, or -1 on error (e.g., null pointer).
 */
int ocre_get_healthcheck_remaining(ocre_healthcheck *WDT);

/**
 * Handles the health check, called cyclically to monitor the container.
 * 
 * @param WDT Pointer to the OCRE health check structure.
 * 
 * @return 1 if health check is alive, 0 otherwise.
 */
int on_ocre_healthcheck(ocre_healthcheck *WDT);

/**
 * Expiry function called by the timer, checks if the container is responsive.
 * 
 * @param WDT Pointer to the OCRE health check structure.
 * 
 * @return 1 if health check failed, 0 otherwise.
 */
int ocre_healthcheck_expiry(ocre_healthcheck *WDT);
#endif