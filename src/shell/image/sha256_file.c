/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "../sha256/sha256.h"

#define FILE_BUFFER_SIZE 1024

int sha256_file(const char *path, char *hash)
{
	int ret = -1;
	void *buffer = NULL;

	if (!path || !hash) {
		return -1;
	}

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	struct stat finfo;
	int rc = fstat(fd, &finfo);
	if (rc < 0) {
		goto finish;
	}

	off_t file_size = finfo.st_size;

	if (!file_size) {
		goto finish;
	}

	buffer = malloc(FILE_BUFFER_SIZE);
	if (!buffer) {
		goto finish;
	}

	struct sha256_buff buff;
	sha256_init(&buff);

	off_t total_bytes_read = 0;
	while (total_bytes_read < file_size) {
		ssize_t bytes_read = read(fd, buffer, FILE_BUFFER_SIZE);
		if (bytes_read < 0) {
			goto finish;
		}

		if (!bytes_read) {
			ret = 0;
			break;
		}

		sha256_update(&buff, buffer, bytes_read);

		total_bytes_read += bytes_read;
	}

	sha256_finalize(&buff);
	sha256_read_hex(&buff, hash);

finish:
	free(buffer);
	close(fd);

	return ret;
}
