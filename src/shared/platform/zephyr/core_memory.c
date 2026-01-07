/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"

#include <zephyr/kernel.h>

void *core_malloc(size_t size) {
    return k_malloc(size);
}

void core_free(void *ptr) {
    k_free(ptr);
}
