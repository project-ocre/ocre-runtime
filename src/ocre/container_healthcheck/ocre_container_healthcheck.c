/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_container_healthcheck.h"
#include "../ocre_container_runtime/ocre_container_runtime.h"

// this function is cyclic called by WDT->timer
int ocre_healthcheck_expiry(ocre_healthcheck *WDT) {

    if (WDT == NULL) {
        return -1; // Error: Null pointer
    }

    if (WDT->enabled && (WDT->is_alive_cnt <= WDT->is_alive_cnt_last)) {
        // TODOA:-- containers[current_container_id].container_runtime_status = CONTAINER_STATUS_UNRESPONSIVE;
        WDT->is_alive_cnt = 0;
        WDT->is_alive_cnt_last = WDT->is_alive_cnt;
        return 1; // Health check failed, container is unresponsive
    }

    WDT->is_alive_cnt_last = WDT->is_alive_cnt;

    return 0; // Health check passed
}

int ocre_healthcheck_init(ocre_healthcheck *WDT, int timeout) {

    if (WDT == NULL) {
        return -1; // Error: Null pointer
    }

    WDT->timeout = timeout;
    WDT->enabled = 1;
    WDT->is_alive_cnt = 0;
    WDT->is_alive_cnt_last = WDT->is_alive_cnt;
    k_timer_init(&WDT->timer, ocre_healthcheck_expiry, NULL);

    return 0;
}

int ocre_healthcheck_reinit(ocre_healthcheck *WDT) {
    
    if (WDT == NULL) {
        return -1; // Error: Null pointer
    }

    WDT->enabled = 1;
    WDT->is_alive_cnt = 0;
    WDT->is_alive_cnt_last = WDT->is_alive_cnt;
    k_timer_init(&WDT->timer, ocre_healthcheck_expiry, NULL);

    return 0;
}

int ocre_healthcheck_restart(ocre_healthcheck *WDT) {
    WDT->is_alive_cnt = 0;
    WDT->is_alive_cnt_last = WDT->is_alive_cnt;
    k_timer_start(&WDT->timer, K_MSEC(WDT->timeout), K_NO_WAIT);
}

int ocre_healthcheck_start(ocre_healthcheck *WDT) {
    k_timer_start(&WDT->timer, K_MSEC(WDT->timeout), K_NO_WAIT);
}

int ocre_healthcheck_stop(ocre_healthcheck *WDT) {
    k_timer_stop(&WDT->timer);
    WDT->enabled = 0;
}

int ocre_get_healthcheck_remaining(ocre_healthcheck *WDT) {
    k_timer_remaining_get(&WDT->timer);
}

// health check of container if it does not respond restart the container
int on_ocre_healthcheck(ocre_healthcheck *WDT) {
    // TODOA:-- k_timer_start(&WDT->timer, K_MSEC(WDT->timeout), K_NO_WAIT);
    //   because the start of already runing timer will get it back to initial values
    //   actually a reset or trigger for watchdog
    //   and the expiry function is not called
    WDT->is_alive_cnt += 1;

    return 1;
}
