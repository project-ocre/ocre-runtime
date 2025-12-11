#include <string.h>
#include <stdint.h>
#include <errno.h>
// #include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <ocre/platform/log.h>
#include <ocre/platform/memory.h>

LOG_MODULE_REGISTER(file_alloc_read, CONFIG_OCRE_LOG_LEVEL);

void *ocre_load_file(const char *path, size_t *size)
{
	if (!path) {
		LOG_ERR("Invalid arguments");
		goto error;
	}

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		LOG_ERR("Failed to open file '%s' errno=%d", path, errno);
		goto error;
	}

	struct stat finfo;
	int rc = fstat(fd, &finfo);
	if (rc < 0) {
		LOG_ERR("Failed to get file info rc=%d errno=%d", rc, errno);
		goto close_file;
	}

	ssize_t file_size = (ssize_t)finfo.st_size;
	//
	// size_t file_size = 100;
	//

	LOG_INF("File size to load: %zu", file_size);

	if (!file_size) {
		LOG_ERR("File is empty");
		goto close_file;
	}

	void *buffer = user_malloc(file_size);
	if (!buffer) {
		LOG_ERR("Failed to allocate memory for file buffer size=%zu errno=%d", file_size, errno);
		goto close_file;
	}

	memset(buffer, 0, file_size);

	// fseek(fp, 0, SEEK_SET);

	// read from file into buffer
	ssize_t bytes_read = read(fd, buffer, file_size);

	if (bytes_read != file_size) {
		LOG_ERR("Failed to read file bytes_read=%zu errno=%d", bytes_read, errno);
		goto free_buffer;
	}

	rc = close(fd);
	if (rc) {
		LOG_ERR("Failed to close file rc=%d errno=%d", rc, errno);
	}

	*size = file_size;

	LOG_INF("File '%s' (size=%zu) loaded successfully", path, file_size);

	return buffer;

free_buffer:
	user_free(buffer);

close_file:
	rc = close(fd);
	if (rc) {
		LOG_ERR("Failed to close file: rc=%d errno=%d", rc, errno);
	}

error:
	return NULL;
}

int ocre_unload_file(void *buffer, size_t size)
{
	user_free(buffer);

	return 0;
}
