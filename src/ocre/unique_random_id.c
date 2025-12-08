#include <stdlib.h>
#include <string.h>

#include "context.h"

#include "unique_random_id.h"

static char nibble_to_hex(char nibble)
{
	return nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
}

static void generate_random_id(char *id, size_t len)
{
	if (len < 1) {
		return;
	}

	size_t i;
	for (i = 0; i < len - 1; i++) {
		id[i] = nibble_to_hex(0xf & rand() / (RAND_MAX / 16));
	}

	id[i] = '\0';
}

int make_unique_random_container_id(const struct ocre_context *context, char *container_id, size_t len)
{
	char *random_id = malloc(len);
	if (!random_id) {
		return -1;
	}

	for (int i = 0; i < 100; i++) {
		generate_random_id(random_id, len);
		if (!ocre_context_get_container_by_id_locked(context, random_id)) {
			strcpy(container_id, random_id);
			free(random_id);
			return 0;
		}
	}

	/* Gave up */
	free(random_id);

	return -1;
}
