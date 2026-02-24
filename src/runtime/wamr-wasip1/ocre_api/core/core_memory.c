/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include "core_external.h"

void *core_malloc(size_t size)
{
	return malloc(size);
}

void core_free(void *ptr)
{
	free(ptr);
}
