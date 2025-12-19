/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>

struct ocre_command {
	char *name;
	int (*func)(struct ocre_context *ctx, const char *argv0, int argc, char **argv);
};
