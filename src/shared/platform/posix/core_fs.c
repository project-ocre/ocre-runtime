/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include "ocre_core_external.h"

#define MAX_PATH_LEN 256

/*
When containers are passed locally, from the filesystem - we don't need to construct the path.
Instead, we just use the original file's location. This is indicated whether the container was passed as a buffer,
or as a program parameter. Encapsulation is used to avoid global variables.
*/
static int stored_argc = 0;

void set_argc(int argc) {
    stored_argc = argc;
}


/**
 * @brief Retrieves the stored argc value.
 *
 * Returns the argc value previously set by set_argc().
 * Useful for conditional logic based on argument count.
 *
 * @return The stored argc value.
 */
static int get_argc() {
    return stored_argc;
}


int core_construct_filepath(char *path, size_t len, char *name) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        // Shall be in /build folder
       LOG_DBG("Current working dir: %s", cwd);
       }
    if (get_argc() > 1) {
        strcpy(path, name);
        return 0;
    } else {
        return snprintf(path, len, "%s/%s/%s.bin", cwd, APP_RESOURCE_PATH, name);
    }
}


int core_filestat(const char *path, size_t *size) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (size) {
            *size = st.st_size;
        }
        return 0;  // success
    } else {
        return errno;  // return the specific error code
    }

}

int core_fileopen(const char *path, void **handle) {
    int *fd = user_malloc(sizeof(int));
    if (!fd) return -ENOMEM;
    *fd = open(path, O_RDONLY);
    if (*fd < 0) {
        user_free(fd);
        return -errno;
    }
    *handle = fd;
    return 0;
}

int core_fileread(void *handle, void *buffer, size_t size) {
    int fd = *(int *)handle;
    ssize_t bytes_read = read(fd, buffer, size);
    if (bytes_read < 0) return -errno;
    if ((size_t)bytes_read != size) return -EIO;
    return 0;
}

int core_fileclose(void *handle) {
    int fd = *(int *)handle;
    int ret = close(fd);
    user_free(handle);
    return ret;
}

static int lsdir(const char *path) {
    DIR *dirp;
    struct dirent *entry;

    dirp = opendir(path);
    if (!dirp) {
        LOG_ERR("Error opening dir %s [%d]", path, errno);
        return -errno;
    }

    while ((entry = readdir(dirp)) != NULL) {
        LOG_DBG("Found: %s", entry->d_name);
    }

    if (closedir(dirp) < 0) {
        LOG_ERR("Error closing dir [%d]", errno);
        return -errno;
    }

    return 0;
}

void ocre_app_storage_init() {
    struct stat st;

    if (stat(OCRE_BASE_PATH, &st) == -1) {
        if (mkdir(OCRE_BASE_PATH, 0755) < 0) {
            LOG_ERR("Failed to create directory %s [%d]", OCRE_BASE_PATH, errno);
        }
    }

    if (stat(APP_RESOURCE_PATH, &st) == -1) {
        if (mkdir(APP_RESOURCE_PATH, 0755) < 0) {
            LOG_ERR("Failed to create directory %s [%d]", APP_RESOURCE_PATH, errno);
        }
    }

    if (stat(PACKAGE_BASE_PATH, &st) == -1) {
        if (mkdir(PACKAGE_BASE_PATH, 0755) < 0) {
            LOG_ERR("Failed to create directory %s [%d]", PACKAGE_BASE_PATH, errno);
        }
    }

    if (stat(CONFIG_PATH, &st) == -1) {
        if (mkdir(CONFIG_PATH, 0755) < 0) {
            LOG_ERR("Failed to create directory %s [%d]", CONFIG_PATH, errno);
        }
    }

#ifdef CONFIG_OCRE_CONTAINER_FILESYSTEM
    if (stat(CONTAINER_FS_PATH, &st) == -1) {
        if (mkdir(CONTAINER_FS_PATH, 0755) < 0) {
            LOG_ERR("Failed to create directory %s [%d]", CONTAINER_FS_PATH, errno);
        }
    }
#endif

    lsdir(OCRE_BASE_PATH);
}
