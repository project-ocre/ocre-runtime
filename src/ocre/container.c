/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <ocre/ocre.h>
#include <ocre/platform/log.h>
#include <ocre/runtime/vtable.h>

#include "ocre.h"
#include "container.h"
#include "util/string_array.h"

LOG_MODULE_REGISTER(container, CONFIG_OCRE_LOG_LEVEL);

struct ocre_container {
	char *id;
	char *image;
	const struct ocre_runtime_vtable *runtime;
	bool detached;
	ocre_container_status_t status;
	pthread_mutex_t mutex;
	sem_t sem_start;
	pthread_cond_t cond_stop;
	void *runtime_context;
	pthread_t thread;
	pthread_attr_t attr;
	struct thread_info *tinfo;
	char **argv;
	char **envp;
	int exit_code;
};

struct container_thread_params {
	int (*func)(void *, sem_t *);
	struct ocre_container *container;
	sem_t *sem;
};

static void *container_thread(void *arg)
{
	int rc;
	struct container_thread_params *params = arg;
	struct ocre_container *container = params->container;

	/* Run the container */

	int result = params->func(container->runtime_context, params->sem);

	/* Exited */

	free(params);

	rc = pthread_mutex_lock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to lock mutex: rc=%d", rc);
		return NULL;
	}

	/* Here is the **only** place where we should set the status to EXITED */

	container->status = OCRE_CONTAINER_STATUS_EXITED;
	container->exit_code = result;

	LOG_INF("Container '%s' exited. Result is = %d", container->id, result);

	/* Notify any waiting threads */

	rc = pthread_cond_broadcast(&container->cond_stop);
	if (rc) {
		LOG_WRN("Failed to broadcast conditional variable: rc=%d", rc);
	}

	rc = pthread_mutex_unlock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
		return NULL;
	}

	/* Cast result int to void pointer so we can return it from the thread */

	return (void *)(intptr_t)result;
}

static ocre_container_status_t ocre_container_status_locked(struct ocre_container *container)
{
	if (!container) {
		LOG_ERR("Invalid container parameter");
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	if (container->status == OCRE_CONTAINER_STATUS_EXITED) {
		/* Need to join the thread to clean up resources and get exit status.
		 * pthread_join should not block here because the thread already exited.
		 * Here is the only place where we should call pthread_join.
		 *
		 * If we later have pthread_join somewhere else, we should not have it here.
		 */

		int rc = pthread_join(container->thread, NULL);
		if (rc) {
			LOG_ERR("Failed to join thread: rc=%d", rc);
			return OCRE_CONTAINER_STATUS_UNKNOWN;
		}

		/* It looks like we should keep the attr available for the whole life of the thread */

		rc = pthread_attr_destroy(&container->attr);
		if (rc) {
			LOG_ERR("Failed to destroy thread attributes: rc=%d", rc);
		}

		/* Now the container is really stopped */

		container->status = OCRE_CONTAINER_STATUS_STOPPED;
	}

	return container->status;
}

static char *ocre_find_best_matching_runtime(const char *image)
{
	static const char default_runtime[] = "wamr/wasip1";

	if (!image) {
		LOG_ERR("Image name cannot be NULL");
		return NULL;
	}

	/* TODO: find the runtime engine from the image manifest, and return the appropriate one */

	/* For now, we use "wamr/wasip1" as default */

	return default_runtime;
}

struct ocre_container *ocre_container_create(const char *img_path, const char *workdir, const char *runtime,
					     const char *container_id, bool detached,
					     const struct ocre_container_args *arguments)
{
	int rc;
	const char **capabilities = NULL;
	const char **mounts = NULL;

	if (!runtime) {
		runtime = ocre_find_best_matching_runtime(img_path);
		if (!runtime) {
			LOG_ERR("Failed to find best matching runtime engine. Please specify a valid runtime engine.");
			return NULL;
		}

		LOG_INF("Selected runtime engine: %s", runtime);
	}

	/* Check mounts parameters of format <source>:<destination>
	 * Destination should be absolute path
	 * We do not like anything to be mounted at '/'
	 * We handle '/' with the 'filesystem' capability.
	 */

	if (arguments && arguments->mounts) {
		const char **mount = arguments->mounts;

		while (*mount) {
			if (*mount[0] != '/') {
				LOG_ERR("Invalid mount format: '%s': source must be absolute path", *mount);
				return NULL;
			}

			char *dst = strchr(*mount, ':');
			if (!dst) {
				LOG_ERR("Invalid mount format: '%s': must be <source>:<destination>", *mount);
				return NULL;
			}

			dst++;

			if (dst[0] != '/') {
				LOG_ERR("Invalid mount format: '%s': destination must be absolute path", *mount);
				return NULL;
			}

			if (dst[1] == '\0') {
				LOG_ERR("Invalid mount format: '%s': destination must not be '/'", *mount);
				return NULL;
			}

			mount++;
		}
	}

	struct ocre_container *container = malloc(sizeof(struct ocre_container));

	if (!container) {
		LOG_ERR("Failed to allocate memory: errno=%d", errno);
		return NULL;
	}

	memset(container, 0, sizeof(struct ocre_container));

	/* Strip the image name from the path, just to make it look nicer */

	const char *image = strrchr(img_path, '/');
	if (image) {
		image++;
	} else {
		image = img_path;
	}

	container->image = strdup(image);
	if (!container->image) {
		LOG_ERR("Failed to allocate memory for image: errno=%d", errno);
		goto error_free;
	}

	container->id = strdup(container_id);
	if (!container->id) {
		LOG_ERR("Failed to allocate memory for id: errno=%d", errno);
		goto error_free;
	}

	/* Duplicate the arguments */

	if (arguments) {
		capabilities = arguments->capabilities;
		mounts = arguments->mounts;

		container->argv = string_array_deep_dup(arguments->argv);
		container->envp = string_array_deep_dup(arguments->envp);

		if ((!container->argv && arguments->argv) || (!container->envp && arguments->envp)) {
			goto error_free;
		}
	}

	container->runtime = ocre_get_runtime(runtime);
	if (!container->runtime) {
		LOG_ERR("Invalid runtime '%s'", runtime);
		goto error_free;
	}

	rc = pthread_mutex_init(&container->mutex, NULL);
	if (rc) {
		LOG_ERR("Failed to initialize mutex: rc=%d", rc);
		goto error_free;
	}

	rc = pthread_cond_init(&container->cond_stop, NULL);
	if (rc) {
		LOG_ERR("Failed to initialize stop conditional variable: rc=%d", rc);
		goto error_mutex;
	}

	rc = sem_init(&container->sem_start, 0, 0);
	if (rc) {
		LOG_ERR("Failed to initialize start semaphore: rc=%d, errno=%d", rc, errno);
		goto error_mutex;
	}

	container->runtime_context =
		container->runtime->create(img_path, workdir, capabilities, (const char **)container->argv,
					   (const char **)container->envp, mounts);
	if (!container->runtime_context) {
		LOG_ERR("Failed to create container");
		goto error_cond;
	}

	container->status = OCRE_CONTAINER_STATUS_CREATED;

	container->detached = detached;

	LOG_INF("Created container '%s' with runtime '%s' (path '%s')", container->id, runtime, img_path);

	return container;

error_cond:
	rc = sem_destroy(&container->sem_start);
	if (rc) {
		LOG_ERR("Failed to deinitialize start semaphore: rc=%d", rc);
	}

	rc = pthread_cond_destroy(&container->cond_stop);
	if (rc) {
		LOG_ERR("Failed to deinitialize stop conditional variable: rc=%d", rc);
	}

error_mutex:
	rc = pthread_mutex_destroy(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to deinitialize mutex: rc=%d", rc);
	}

error_free:
	string_array_free(container->argv);
	string_array_free(container->envp);

	free(container->image);
	free(container->id);
	free(container);

	return NULL;
}

int ocre_container_destroy(struct ocre_container *container)
{
	if (!container) {
		LOG_ERR("Invalid arguments");
		return -1;
	}

	ocre_container_status_t status = ocre_container_status_locked(container);
	if (status != OCRE_CONTAINER_STATUS_STOPPED && status != OCRE_CONTAINER_STATUS_CREATED &&
	    status != OCRE_CONTAINER_STATUS_ERROR) {
		LOG_ERR("Cannot remove container '%s' because it is in use", container->id);
		return -1;
	}

	container->runtime->destroy(container->runtime_context);

	int rc;
	rc = pthread_mutex_destroy(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to deinitialize mutex: rc=%d", rc);
	}

	rc = sem_destroy(&container->sem_start);
	if (rc) {
		LOG_ERR("Failed to deinitialize start semaphore: rc=%d", rc);
	}

	rc = pthread_cond_destroy(&container->cond_stop);
	if (rc) {
		LOG_ERR("Failed to deinitialize stop conditional variable: rc=%d", rc);
	}

	string_array_free(container->argv);
	string_array_free(container->envp);

	LOG_INF("Removed container '%s'", container->id);

	free(container->id);
	free(container->image);
	free(container);

	return 0;
}

int ocre_container_start(struct ocre_container *container)
{
	if (!container) {
		LOG_ERR("Invalid arguments");
		return -1;
	}

	int rc;
	rc = pthread_mutex_lock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to lock mutex: rc=%d", rc);
		goto error_status;
	}

	ocre_container_status_t status = ocre_container_status_locked(container);
	if (status != OCRE_CONTAINER_STATUS_CREATED && status != OCRE_CONTAINER_STATUS_STOPPED) {
		LOG_ERR("Container '%s' is not ready to run", container->id);
		goto error_mutex;
	}

	rc = pthread_attr_init(&container->attr);
	if (rc) {
		LOG_ERR("Failed to initialize pthread attribute: rc=%d", rc);
		goto error_mutex;
	}

	struct container_thread_params *params;
	params = malloc(sizeof(struct container_thread_params));
	if (!params) {
		LOG_ERR("Failed to allocate memory (size=%zu) for thread parameters: errno=%d",
			sizeof(struct container_thread_params), errno);
		goto error_attr;
	}

	memset(params, 0, sizeof(struct container_thread_params));
	params->container = container;
	params->func = container->runtime->thread_execute;
	params->sem = &container->sem_start;
	container->status = OCRE_CONTAINER_STATUS_RUNNING;

	rc = pthread_create(&container->thread, &container->attr, container_thread, params);
	if (rc) {
		LOG_ERR("Failed to create thread: rc=%d", rc);
		goto error_params;
	}

	LOG_INF("Waiting for container '%s' to start", container->id);

	rc = pthread_mutex_unlock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
		goto error_status;
	}

	rc = sem_wait(&container->sem_start);
	if (rc) {
		LOG_ERR("Failed to wait on start semaphore: rc=%d", rc);
		goto error_status;
	}

	LOG_INF("Started container '%s' on runtime '%s'", container->id, container->runtime->runtime_name);

	if (!container->detached) {
		/* This will block until the container thread exits */

		if (ocre_container_wait(container, NULL)) {
			LOG_ERR("Failed to wait for container '%s'", container->id);
			return -1;
		}
	}

	return 0;

error_params:
	free(params);

error_attr:
	rc = pthread_attr_destroy(&container->attr);
	if (rc) {
		LOG_ERR("Failed to destroy thread attributes: rc=%d", rc);
	}

error_mutex:
	rc = pthread_mutex_unlock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
	}

error_status:
	LOG_INF("Setting container '%s' status to ERROR", container->id);
	container->status = OCRE_CONTAINER_STATUS_ERROR;

	return -1;
}

ocre_container_status_t ocre_container_get_status(struct ocre_container *container)
{
	if (!container) {
		LOG_ERR("Invalid arguments");
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	int rc;
	rc = pthread_mutex_lock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to lock mutex: rc=%d", rc);
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	ocre_container_status_t status = ocre_container_status_locked(container);

	rc = pthread_mutex_unlock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	return status;
}

int ocre_container_stop(struct ocre_container *container)
{
	int ret = -1;
	if (!container) {
		LOG_ERR("Invalid container");
		return -1;
	}

	int rc;
	rc = pthread_mutex_lock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to lock mutex: rc=%d", rc);
		return -1;
	}

	if (container->status != OCRE_CONTAINER_STATUS_RUNNING) {
		LOG_ERR("Container '%s' is not running", container->id);
		goto unlock_mutex;
	}

	if (!container->runtime->stop) {
		LOG_ERR("Container '%s' does not support stop", container->id);
		goto unlock_mutex;
	}

	LOG_INF("Sending stop signal to container '%s'", container->id);

	ret = container->runtime->stop(container->runtime_context);

unlock_mutex:
	rc = pthread_mutex_unlock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
	}

	return ret;
}

int ocre_container_kill(struct ocre_container *container)
{
	int ret = -1;
	if (!container) {
		LOG_ERR("Invalid container");
		return -1;
	}

	int rc;
	rc = pthread_mutex_lock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to lock mutex: rc=%d", rc);
		return -1;
	}

	if (container->status != OCRE_CONTAINER_STATUS_RUNNING) {
		LOG_ERR("Container '%s' is not running", container->id);
		goto unlock_mutex;
	}

	ret = container->runtime->kill(container->runtime_context);

	LOG_INF("Sent kill signal to container '%s'", container->id);

unlock_mutex:
	rc = pthread_mutex_unlock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
	}

	return ret;
}

int ocre_container_pause(struct ocre_container *container)
{
	int ret = -1;
	if (!container) {
		LOG_ERR("Invalid container");
		return -1;
	}

	int rc;
	rc = pthread_mutex_lock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to lock mutex: rc=%d", rc);
		return -1;
	}

	if (container->status != OCRE_CONTAINER_STATUS_RUNNING) {
		LOG_ERR("Container '%s' is not running", container->id);
		goto unlock_mutex;
	}

	if (!container->runtime->pause) {
		LOG_ERR("Container '%s' does not support pause", container->id);
		goto unlock_mutex;
	}

	LOG_INF("Sending pause signal to container '%s'", container->id);

	ret = container->runtime->pause(container->runtime_context);

	if (ret) {
		LOG_ERR("Failed to pause container '%s': rc=%d", container->id, ret);
		goto unlock_mutex;
	}

	container->status = OCRE_CONTAINER_STATUS_PAUSED;

unlock_mutex:
	rc = pthread_mutex_unlock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
	}

	return ret;
}

int ocre_container_unpause(struct ocre_container *container)
{
	int ret = -1;
	if (!container) {
		LOG_ERR("Invalid container");
		return -1;
	}

	int rc;
	rc = pthread_mutex_lock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to lock mutex: rc=%d", rc);
		return -1;
	}

	if (container->status != OCRE_CONTAINER_STATUS_PAUSED) {
		LOG_ERR("Container '%s' is not paused", container->id);
		goto unlock_mutex;
	}

	if (!container->runtime->unpause) {
		LOG_ERR("Container '%s' does not support unpause", container->id);
		goto unlock_mutex;
	}

	LOG_INF("Sending unpause signal to container '%s'", container->id);

	ret = container->runtime->unpause(container->runtime_context);

	if (ret) {
		LOG_ERR("Failed to unpause container '%s': rc=%d", container->id, ret);
		goto unlock_mutex;
	}

	container->status = OCRE_CONTAINER_STATUS_RUNNING;

unlock_mutex:
	rc = pthread_mutex_unlock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
	}

	return ret;
}

int ocre_container_wait(struct ocre_container *container, int *status)
{
	int ret = -1;

	if (!container) {
		LOG_ERR("Invalid container");
		return -1;
	}

	int rc;
	rc = pthread_mutex_lock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to lock mutex: rc=%d", rc);
		return -1;
	}

	if (container->status == OCRE_CONTAINER_STATUS_UNKNOWN || container->status == OCRE_CONTAINER_STATUS_CREATED) {
		ret = -1;
		goto unlock_mutex;
	}

	if (container->status == OCRE_CONTAINER_STATUS_RUNNING) {
		LOG_INF("Container '%s' is running", container->id);
		rc = pthread_cond_wait(&container->cond_stop, &container->mutex);
		if (rc) {
			LOG_ERR("Failed to wait on stop conditional variable: rc=%d", rc);
			goto unlock_mutex;
		}
	}

	if (container->status == OCRE_CONTAINER_STATUS_EXITED) {
		LOG_INF("Container '%s' was exited", container->id);
		if (ocre_container_status_locked(container) != OCRE_CONTAINER_STATUS_STOPPED) {
			LOG_ERR("Container '%s' status did not go from exited to stopped", container->id);
			goto unlock_mutex;
		}
	}

	ret = 0;

	if (container->status == OCRE_CONTAINER_STATUS_STOPPED) {
		LOG_INF("Container '%s' was stopped", container->id);
		if (status) {
			*status = container->exit_code;
		}
	}

unlock_mutex:
	rc = pthread_mutex_unlock(&container->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
	}

	return ret;
}

const char *ocre_container_get_id(const struct ocre_container *container)
{
	if (!container) {
		LOG_ERR("Invalid container or id");
		return NULL;
	}

	return container->id;
}

const char *ocre_container_get_image(const struct ocre_container *container)
{
	if (!container) {
		LOG_ERR("Invalid container or id");
		return NULL;
	}

	return container->image;
}
