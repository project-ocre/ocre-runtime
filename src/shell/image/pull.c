#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <ocre/ocre.h>

#include "../command.h"
#include "sha256_file.h"

extern int ocre_download_file(const char *url, const char *filepath);

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s image pull URL\n", argv0); // TODO: NAME[:TAG|@DIGEST]
	fprintf(stderr, "\nDownloads an image from a remote repository to the local storage.\n");
	return -1;
}

/* cppcheck-suppress constParameterPointer */
int cmd_image_pull(struct ocre_context *ctx, const char *argv0, int argc, char **argv)
{
	char *local_name = NULL;
	int ret = -1;

	if (argc < 2) {
		fprintf(stderr, "'%s image pull' requires at least one argument\n\n", argv0);
		return usage(argv0);
	}

	if (argc == 2) {
		local_name = strrchr(argv[1], '/');
		if (local_name == NULL) {
			fprintf(stderr, "'Cannot determine image name from URL '%s'\n", argv[1]);
			return -1;
		} else {
			local_name++;
		}
		if (*local_name == '\0') {
			fprintf(stderr, "'Cannot determine image name from URL '%s'\n", argv[1]);
			return -1;
		}

	} else {
		local_name = argv[2];
	}

	/* Check if the provided image ID is valid */

	if (!ocre_is_valid_id(local_name)) {
		fprintf(stderr,
			"Invalid characters in image ID '%s'. Valid are [a-z0-9_-.] (lowercase "
			"alphanumeric) and cannot start with '.'\n",
			local_name);
		return -1;
	}

	const char *working_directory = ocre_context_get_working_directory(ctx);

	char *image_path = malloc(strlen(working_directory) + strlen("/images/") + strlen(local_name) + 1);
	if (!image_path) {
		fprintf(stderr, "Failed to allocate memory for image directory path\n");
		return -1;
	}

	strcpy(image_path, working_directory);
	strcat(image_path, "/images/");
	strcat(image_path, local_name);

	/* Check if image already exists */

	struct stat st;

	ret = stat(image_path, &st);
	if (!ret) {
		fprintf(stderr, "Image '%s' already exists\n", local_name);
		goto finish;
	}

	fprintf(stderr, "Pulling '%s' from '%s'\n", local_name, argv[1]);

	ret = ocre_download_file(argv[1], image_path);
	if (ret) {
		fprintf(stderr, "Failed to download image '%s'\n", argv[1]);
		goto finish;
	}

	char hash[65] = {0}; /* hash[64] is null-byte */

	sha256_file(image_path, hash);

	fprintf(stderr, "Digest: %s\n", hash);

	fprintf(stdout, "%s\n", local_name);

finish:
	free(image_path);

	return ret;
}
