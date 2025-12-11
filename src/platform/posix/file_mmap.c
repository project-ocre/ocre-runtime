#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <ocre/platform/log.h>
#include <ocre/platform/memory.h>

LOG_MODULE_REGISTER(file_mmap, CONFIG_OCRE_LOG_LEVEL);

void *ocre_load_file(const char *path, size_t *size)
{
	int save_errno = 0;
	void *buffer = NULL;

	if (!path) {
		LOG_ERR("Invalid arguments");
		save_errno = EINVAL;
		goto error_errno;
	}

	FILE *fp = fopen(path, "rb");
	if (!fp) {
		save_errno = errno;
		LOG_ERR("Failed to open file '%s': errno=%d", path, save_errno);
		goto error_errno;
	}

	int fd = fileno(fp);
	if (fd < 0) {
		save_errno = errno;
		LOG_ERR("Failed to get file descriptor (fd=%d): errno=%d", fd, save_errno);
		goto error_close;
	}

	struct stat finfo;
	int rc = fstat(fd, &finfo);
	if (rc < 0) {
		save_errno = errno;
		LOG_ERR("Failed to get file info: rc=%d errno=%d", rc, save_errno);
		goto error_close;
	}

	size_t file_size = finfo.st_size;
	if (!file_size) {
		LOG_WRN("File is empty");
	}

	buffer = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (!buffer || buffer == MAP_FAILED) {
		save_errno = errno;
		LOG_ERR("Failed to mmap file %zu errno=%d", file_size, errno);
		goto error_close;
	}

	if (fclose(fp) != 0) {
		LOG_ERR("Failed to close file errno=%d", errno);
	}

	*size = file_size;

	LOG_INF("File '%s' (size=%zu) mmapped successfully", path, file_size);

	return buffer;

error_close:
	fclose(fp);

error_errno:
	errno = save_errno;

	return NULL;
}

int ocre_unload_file(void *buffer, size_t size)
{
    if (!buffer) {
        return 0;
    }

	if (munmap(buffer, size)) {
		int save_errno = errno;
		LOG_ERR("Failed to munmap file errno=%d", save_errno);
		errno = save_errno;
		return -1;
	}

	return 0;
}
