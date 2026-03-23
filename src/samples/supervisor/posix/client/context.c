/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>

int ocre_initialize(const struct ocre_runtime_vtable *const vtable[])
{
	return 0;
}

void ocre_deinitialize(void)
{
	return;
}

struct ocre_context *ocre_create_context(const char *workdir)
{
	return (struct ocre_context *)0x31337;
}

int ocre_destroy_context(struct ocre_context *context)
{
	if (!context) {
		return -1;
	}

	return 0;
}
