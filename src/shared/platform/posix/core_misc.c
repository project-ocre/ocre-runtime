/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"

#include <time.h>
#include <errno.h>  
#include <sched.h>
#include <stdio.h>

void core_sleep_ms(int milliseconds)
{
    struct timespec ts;
    int res;

    if (milliseconds < 0)
    {
        errno = EINVAL;
        fprintf(stderr, "core_sleep_ms: Invalid milliseconds value (%d)\n", milliseconds);
        return;
    }

    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
        if (res && errno != EINTR) {
            fprintf(stderr, "core_sleep_ms: nanosleep failed (errno=%d)\n", errno);
        }
    } while (res && errno == EINTR);
}

void core_yield(void) {
    sched_yield();
}
