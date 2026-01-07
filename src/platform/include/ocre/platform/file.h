/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

void *ocre_load_file(const char *path, size_t *size);
int ocre_unload_file(void *buffer, size_t size);
