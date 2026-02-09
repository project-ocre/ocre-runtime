#include <stddef.h>
#include <ctype.h>
#include <string.h>

int ocre_is_valid_id(const char *id)
{
	/* Cannot be NULL */

	if (!id) {
		return 0;
	}

	/* Cannot be empty or start with a dot '.' */

	if (id[0] == '\0' || id[0] == '.') {
		return 0;
	}

	/* Can only contain alphanumeric characters, dots, underscores, and hyphens */

	for (size_t i = 0; i < strlen(id); i++) {
		if ((isalnum((int)id[i]) && islower((int)id[i])) || id[i] == '.' || id[i] == '_' || id[i] == '-') {
			continue;
		}

		return 0;
	}

	return 1;
}
