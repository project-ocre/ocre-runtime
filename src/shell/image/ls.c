#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include <ocre/ocre.h>

#include "../command.h"
#include "sha256_file.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s image ls [IMAGE]\n", argv0);
	fprintf(stderr, "\nList images in local storage.\n");
	return -1;
}

static void header()
{
	printf("SHA-256\t\t\t\t\t\t\t\t\tSIZE\tNAME\n");
}

static int list_image(const char *name, const char *path)
{
	struct stat st;

	if (!path) {
		fprintf(stderr, "Invalid image path\n");
		return -1;
	}

	if (stat(path, &st)) {
		fprintf(stderr, "Failed to stat image '%s'\n", path);
		return -1;
	}

	if (S_ISREG(st.st_mode)) {
		char hash[65] = {0}; /* hash[64] is null-byte */

		sha256_file(path, hash);

		printf("%s\t%ju\t%s\n", hash, (uintmax_t)st.st_size, name);
	}

	return 0;
}

static int list_images(const char *path)
{
	int ret = 0;
	DIR *d;
	const struct dirent *dir;

	d = opendir(path);
	if (!d) {
		fprintf(stderr, "Failed to open directory '%s'\n", path);
		return -1;
	}

	while ((dir = readdir(d)) != NULL) {
		char *image_path = malloc(strlen(path) + strlen(dir->d_name) + 2);
		if (!image_path) {
			fprintf(stderr, "Failed to allocate memory for image path\n");
			ret = -1;
			break;
		}

		strcpy(image_path, path);
		strcat(image_path, "/");
		strcat(image_path, dir->d_name);

		if (list_image(dir->d_name, image_path)) {
			fprintf(stderr, "Failed to list image '%s'\n", image_path);
			ret = -1;
		}

		free(image_path);
	}

	closedir(d);

	return ret;
}

int cmd_image_ls(struct ocre_context *ctx, const char *argv0, int argc, char **argv)
{
	switch (argc) {
		case 1: {
			const char *working_directory = ocre_context_get_working_directory(ctx);

			char *image_dir = malloc(strlen(working_directory) + strlen("/images") + 1);
			if (!image_dir) {
				fprintf(stderr, "Failed to allocate memory for image directory path\n");
				return -1;
			}

			strcpy(image_dir, working_directory);
			strcat(image_dir, "/images");

			header();

			if (list_images(image_dir)) {
				fprintf(stderr, "Failed to list images in directory '%s'\n", image_dir);
			}

			free(image_dir);

			break;
		}
		case 2: {
			const char *working_directory = ocre_context_get_working_directory(ctx);

			/* Check if the provided image ID is valid */

			if (argv[1] && !ocre_is_valid_id(argv[1])) {
				fprintf(stderr,
					"Invalid characters in image ID '%s'. Valid are [a-z0-9_-.] (lowercase "
					"alphanumeric) and cannot start with '.'\n",
					argv[1]);
				return -1;
			}

			char *image_path = malloc(strlen(working_directory) + strlen("/images") + strlen(argv[1]) + 2);
			if (!image_path) {
				fprintf(stderr, "Failed to allocate memory for image path\n");
				return -1;
			}

			strcpy(image_path, working_directory);
			strcat(image_path, "/images/");
			strcat(image_path, argv[1]);

			header();

			if (list_image(argv[1], image_path)) {
				fprintf(stderr, "Failed to list image '%s'\n", image_path);
			}

			free(image_path);

			break;
		}
		default: {
			fprintf(stderr, "'%s image ls' requires at most one argument\n\n", argv0);
			return usage(argv0);
		}
	}

	return 0;
}
