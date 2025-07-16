/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"

#include <stdlib.h>

void *core_malloc(size_t size) {
    return malloc(size);
}

void core_free(void *ptr) {
    free(ptr);
}
