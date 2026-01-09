/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include <ocre/ocre.h>

#ifdef CONFIG_ARCH_POSIX
#include "nsi_main.h"
#endif

extern const unsigned char ocre_mini_sample_image[];
extern const size_t ocre_mini_sample_image_len;

static int create_sample_file(char *path)
{
	int fd = open(path, O_CREAT | O_WRONLY, 0644);
	if (fd == -1) {
		perror("Failed to create sample file");
		return -1;
	}

	ssize_t bytes_written = write(fd, ocre_mini_sample_image, ocre_mini_sample_image_len);
	if (bytes_written != ocre_mini_sample_image_len) {
		perror("Failed to write to sample file");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int main(int argc, char *argv[])
{
	int rc;

	rc = ocre_initialize(NULL);
	if (rc) {
		fprintf(stderr, "Failed to initialize runtimes\n");
		return 1;
	}

	struct ocre_context *ocre = ocre_create_context(NULL);
	if (!ocre) {
		fprintf(stderr, "Failed to create ocre context\n");
		return 1;
	}

	const char *workdir = ocre_context_get_working_directory(ocre);
	if (!workdir) {
		fprintf(stderr, "Failed to get working directory\n");
		return 1;
	}

	char *sample_path = malloc(strlen(workdir) + strlen("/images/sample.wasm") + 1);
	if (!sample_path) {
		fprintf(stderr, "Failed to allocate memory for sample path\n");
		return 1;
	}
	sprintf(sample_path, "%s/images/sample.wasm", workdir);

	struct stat st;

	if (stat(sample_path, &st) == -1) {
		if (errno == ENOENT) {
			fprintf(stderr, "Creating sample file '%s'\n", sample_path);
			if (create_sample_file(sample_path)) {
				fprintf(stderr, "Failed to create sample file\n");
				free(sample_path);
				return 1;
			}
		} else {
			perror("Failed to stat sample file");
			free(sample_path);
			return 1;
		}
	} else {
		fprintf(stderr, "Sample file '%s' already exists\n", sample_path);
	}

	free(sample_path);

	struct ocre_container *container =
		ocre_context_create_container(ocre, "sample.wasm", "wamr/wasip1", NULL, false, NULL);

	if (!container) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	rc = ocre_container_start(container);
	if (rc) {
		fprintf(stderr, "Failed to start container\n");
		return 1;
	}

	rc = ocre_context_remove_container(ocre, container);
	if (rc) {
		fprintf(stderr, "Failed to remove container\n");
		return 1;
	}

	ocre_destroy_context(ocre);

	ocre_deinitialize();

	/* Exit simulator on zephyr */

#ifdef CONFIG_ARCH_POSIX
	nsi_exit(0);
#endif

	return 0;
}
