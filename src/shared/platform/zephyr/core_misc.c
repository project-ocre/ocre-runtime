/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"
#include <zephyr/kernel.h>

void core_sleep_ms(int milliseconds) {
    k_msleep(milliseconds);
}

void core_yield(void) {
    k_yield();
}
