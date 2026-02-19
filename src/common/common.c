#include <stddef.h>
#include <ctype.h>
#include <string.h>

#include <ocre/common.h>

#include "version.h"
#include "build_info.h"
#include "commit_id.h"

/* Constant build information */

const struct ocre_config ocre_build_configuration = {
	.build_info = OCRE_BUILD_HOST_INFO,
	.version = OCRE_VERSION_STRING,
	.commit_id = GIT_COMMIT_ID,
	.build_date = OCRE_BUILD_DATE,
};

int ocre_is_valid_name(const char *id)
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
