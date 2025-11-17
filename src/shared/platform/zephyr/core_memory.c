/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"

#include <zephyr/kernel.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

void *core_malloc(size_t size) {
    return k_malloc(size);
}

void core_free(void *ptr) {
    k_free(ptr);
}

#ifdef CONFIG_SHARED_MULTI_HEAP
void *user_malloc(size_t size) {
    return shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, 32, size);
}

void user_free(void *ptr) {
    shared_multi_heap_free(ptr);
}
#else
#warning CONFIG_SHARED_MULTI_HEAP is not defined. Using internal RAM
void *user_malloc(size_t size) {
    return k_malloc(size);
}

void user_free(void *ptr) {
    k_free(ptr);
}
#endif
