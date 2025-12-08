#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uthash/utlist.h>

#include <ocre/ocre.h>
#include <ocre/platform/config.h>
#include <ocre/platform/log.h>

#include "container.h"
#include "unique_random_id.h"

#define RANDOM_ID_LEN 8

LOG_MODULE_REGISTER(context);

struct ocre_context {
	pthread_mutex_t mutex;
	char *working_directory;
	struct container_node *containers;
};

struct container_node {
	struct ocre_container *container;
	struct container_node *next; /* needed for singly- or doubly-linked lists */
};

struct ocre_context *ocre_create_context(const char *workdir)
{
	int rc;

	if (!workdir) {
		workdir = CONFIG_OCRE_DEFAULT_WORKING_DIRECTORY;
		LOG_INF("Using default working directory: %s", workdir);
	}

	struct ocre_context *context = malloc(sizeof(struct ocre_context));
	if (!context) {
		LOG_ERR("Failed to allocate memory for context: errno=%d", errno);
		return NULL;
	}

	memset(context, 0, sizeof(struct ocre_context));

	/* Initialize working directory */

	context->working_directory = strdup(workdir);
	if (!context->working_directory) {
		LOG_ERR("Failed to allocate memory for working directory: errno=%d", errno);
		free(context);
		goto error_context;
	}

	rc = pthread_mutex_init(&context->mutex, NULL);
	if (rc) {
		LOG_ERR("Failed to initialize context mutex: rc=%d", rc);
		goto error_workdir;
	}

	/* Initialize containers list */

	context->containers = NULL;

	return context;

error_workdir:
	free(context->working_directory);

error_context:
	free(context);

	return NULL;
};

void ocre_destroy_context(struct ocre_context *context)
{
	pthread_mutex_destroy(&context->mutex);
	free(context->working_directory);
	free(context);
}

struct ocre_container *ocre_context_get_container_by_id_locked(const struct ocre_context *context, const char *id)
{
	struct container_node *node;

	LL_FOREACH(context->containers, node)
	{
		if (!ocre_container_id_compare(node->container, id)) {
			return node->container;
		}
	}

	return NULL;
}

void ocre_context_destroy(struct ocre_context *context)
{
	struct container_node *node;

	if (!context) {
		LOG_ERR("Invalid context");
		return;
	}

	/* Send kill event to all containers */

	LL_FOREACH(context->containers, node)
	{
		ocre_container_kill(node->container);
	}

	/* Wait for all containers to exit */

	LL_FOREACH(context->containers, node)
	{
		ocre_container_wait(node->container, NULL);
	}

	int rc;
	rc = pthread_mutex_destroy(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to destroy context mutex: rc=%d", rc);
	}

	free(context->working_directory);

	free(context);
};

struct ocre_container *ocre_context_create_container(struct ocre_context *context, const char *image,
						     const char *const runtime, const char *container_id, bool detached,
						     const struct ocre_container_args *arguments)
{
	const char *computed_container_id = NULL;
	struct ocre_container *container = NULL;
	int rc;

	rc = pthread_mutex_lock(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to lock context mutex: rc=%d", rc);
		return NULL;
	}

	/* If no container ID is provided, generate a random one */

	char random_id[RANDOM_ID_LEN];

	if (!container_id) {
		if (make_unique_random_container_id(context, random_id, RANDOM_ID_LEN)) {
			LOG_ERR("Failed to generate random container ID");
			goto unlock_mutex;
		}

		computed_container_id = random_id;
	} else if (ocre_context_get_container_by_id_locked(context, container_id)) {
		LOG_ERR("Container with ID '%s' already exists", container_id);
		goto unlock_mutex;
	} else {
		computed_container_id = container_id;
	}

	/* Build the absolute path to the image */

	char *abs_path = malloc(strlen(context->working_directory) + strlen("/images/") + strlen(image) + 1);
	if (!abs_path) {
		LOG_ERR("Failed to allocate memory for absolute path");
		goto unlock_mutex;
	}

	strcpy(abs_path, context->working_directory);
	strcat(abs_path, "/images/");
	strcat(abs_path, image);

	/* Create the container */

	container = ocre_container_create(abs_path, runtime, computed_container_id, detached, arguments);
	if (!container) {
		LOG_ERR("Failed to create container %s: errno=%d", computed_container_id, errno);
		goto free_path;
	}

	/* Add the container to the context */

	struct container_node *node = malloc(sizeof(struct container_node));
	if (!node) {
		LOG_ERR("Failed to allocate memory for container node");
		goto error_container;
	}

	node->container = container;

	LL_APPEND(context->containers, node);

	goto free_path;

error_container:
	ocre_container_destroy(container);

free_path:
	free(abs_path);

unlock_mutex:
	rc = pthread_mutex_unlock(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock context mutex: rc=%d", rc);
		return NULL;
	}

	return container;
}

int ocre_context_remove_container(struct ocre_context *context, struct ocre_container *container)
{
	struct container_node *node = NULL;

	if (!context || !container) {
		LOG_ERR("Invalid arguments");
		return -1;
	}

	LL_FOREACH(context->containers, node)
	{
		if (node->container == container) {
			int rc = ocre_container_destroy(container);
			if (rc) {
				LOG_ERR("Failed to destroy container: rc=%d", rc);
				return rc;
			}

			LL_DELETE(context->containers, node);
			free(node);

			return 0;
		}
	}

	return -1;
}

int ocre_context_get_num_containers(struct ocre_context *context)
{
	int rc;
	int count = 0;
	struct container_node *node = NULL;

	if (!context) {
		LOG_ERR("Invalid context");
		return -1;
	}

	rc = pthread_mutex_lock(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to lock context mutex: rc=%d", rc);
		return -1;
	}

	LL_COUNT(context->containers, node, count);

	rc = pthread_mutex_unlock(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock context mutex: rc=%d", rc);
		return -1;
	}

	return count;
}

char *ocre_context_get_working_directory(struct ocre_context *context) {
    /* We never change this, no need to lock */

    return context->working_directory;
}

int ocre_context_list_containers(struct ocre_context *context, struct ocre_container **containers, int max_size)
{
	int rc;
	int count = 0;
	struct container_node *node = NULL;

	if (!context || !containers) {
		LOG_ERR("Invalid arguments");
		return -1;
	}

	rc = pthread_mutex_lock(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to lock context mutex: rc=%d", rc);
		return -1;
	}

	LL_FOREACH(context->containers, node)
	{
		if (count >= max_size) {
			break;
		}

		containers[count++] = node->container;
	}

	rc = pthread_mutex_unlock(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock context mutex: rc=%d", rc);
		return -1;
	}

	return count;
}

struct ocre_container *ocre_context_get_container_by_id(struct ocre_context *context, const char *id)
{
	int rc;

	if (!context || !id) {
		LOG_ERR("Invalid arguments");
		return NULL;
	}

	rc = pthread_mutex_lock(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to lock context mutex: rc=%d", rc);
		return NULL;
	}

	struct ocre_container *ret = ocre_context_get_container_by_id_locked(context, id);

	rc = pthread_mutex_unlock(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to unlock context mutex: rc=%d", rc);
		return NULL;
	}

	return ret;
}
