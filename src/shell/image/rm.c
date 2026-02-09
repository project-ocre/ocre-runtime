/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ocre/ocre.h>

#include "../command.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s image rm <IMAGE>\n", argv0);
	fprintf(stderr, "\nRemoves an image from local storage.\n");
	return -1;
}

/* cppcheck-suppress constParameterPointer */
int cmd_image_rm(struct ocre_context *ctx, const char *argv0, int argc, char **argv)
{
	if (argc == 2) {
		/* Check if the provided image ID is valid */

		if (argv[1] && !ocre_is_valid_name(argv[1])) {
			fprintf(stderr,
				"Invalid characters in image ID '%s'. Valid are [a-z0-9_-.] (lowercase "
				"alphanumeric) and cannot start with '.'\n",
				argv[1]);
			return -1;
		}

		const char *working_directory = ocre_context_get_working_directory(ctx);

		char *image_path = malloc(strlen(working_directory) + strlen("/images") + strlen(argv[1]) + 2);
		if (!image_path) {
			fprintf(stderr, "Failed to allocate memory for image path\n");
			return -1;
		}

		strcpy(image_path, working_directory);
		strcat(image_path, "/images/");
		strcat(image_path, argv[1]);

		/* Danger: we do not check if the image is in use */

		if (remove(image_path)) {
			fprintf(stderr, "Failed to remove image '%s'\n", image_path);
		}

		free(image_path);
	} else {
		fprintf(stderr, "'%s image rm' requires exactly one argument\n\n", argv0);
		return usage(argv0);
	}

	return 0;
}
