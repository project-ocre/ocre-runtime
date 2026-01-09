/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "string_array.h"

size_t string_array_size(const char **array)
{
	size_t size = 0;

	if (!array) {
		return 0;
	}

	do {
		size++;
	} while (*array++);

	return size;
}

void string_array_free(char **array)
{
	if (!array) {
		return;
	}

	for (size_t j = 0; array[j]; j++) {
		free(array[j]);
	}

	free(array);
}

char **string_array_deep_dup(const char **const src)
{
	size_t size = string_array_size((const char **)src);

	if (!size) {
		return NULL;
	}

	char **dst = malloc(size * sizeof(char *));
	if (!dst) {
		return NULL;
	}

	memset(dst, 0, size * sizeof(char *));

	size_t i;
	for (i = 0; i < size - 1; i++) {
		dst[i] = strdup(src[i]);
		if (!dst[i]) {
			goto error;
		}
	}

	dst[i] = NULL;

	return dst;

error:
	string_array_free(dst);

	return NULL;
}

const char *string_array_lookup(const char **array, const char *key)
{
	if (!array || !key) {
		return NULL;
	}

	for (size_t i = 0; array[i]; i++) {
		if (!strcmp(array[i], key)) {
			return array[i];
		}
	}

	return NULL;
}
