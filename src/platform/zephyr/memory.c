/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

#ifdef CONFIG_SHARED_MULTI_HEAP
void *user_malloc(size_t size)
{
	return shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, 32, size);
}

void user_free(void *ptr)
{
	shared_multi_heap_free(ptr);
}

void *user_realloc(void *ptr, size_t size)
{
	// TODO
	return NULL;
}

#else

#warning CONFIG_SHARED_MULTI_HEAP is not defined. Using internal RAM

void *user_malloc(size_t size)
{
	return malloc(size);
}

void user_free(void *ptr)
{
	free(ptr);
}

void *user_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

#endif
