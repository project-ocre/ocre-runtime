#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "string_array.h"

// returns the size of a string array including the NULL termination
// returns 0 if pointer is null
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

// duplicates an array of char *
// returns NULL on error
char **string_array_dup(char **src)
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
	for (i = 0; i < size; i++) {
		dst[i] = src[i];
	}

	return dst;
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

// makes a deep copy of an array of char *
// returns NULL on error
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

size_t string_array_copy(char **dest, char **src)
{
	if (!dest || !src) {
		return 0;
	}

	size_t count = 0;
	do {
		dest[count] = src[count];
	} while (src[count]);

	return count;
}
