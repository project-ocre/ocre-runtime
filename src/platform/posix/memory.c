/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/platform/memory.h>
#include <stdlib.h>

void *user_malloc(size_t size)
{
	return malloc(size);
}

void user_free(void *p)
{
	free(p);
}

void *user_realloc(void *p, size_t size)
{
	return realloc(p, size);
}
