#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <uthash/utlist.h>

#include <ocre/ocre.h>
#include <ocre/platform/config.h>
#include <ocre/platform/log.h>

#include "container.h"
#include "unique_random_id.h"
#include "util/rm_rf.h"
#include "util/string_array.h"

#define RANDOM_ID_LEN 8

LOG_MODULE_REGISTER(context, CONFIG_OCRE_LOG_LEVEL);

struct ocre_context {
	pthread_mutex_t mutex;
	char *working_directory;
	struct container_node *containers;
};

struct container_node {
	struct ocre_container *container;
	char *working_directory;
	struct container_node *next; /* needed for singly- or doubly-linked lists */
};

#if CONFIG_OCRE_FILESYSTEM
static int delete_container_workdirs(const char *working_directory)
{
	int ret = -1;
	DIR *d = NULL;
	struct dirent *dir = NULL;

	char *containers_path = malloc(strlen(working_directory) + strlen("/containers") + 1);
	if (!containers_path) {
		LOG_ERR("Failed to allocate memory for container workdir path");
		return -1;
	}

	sprintf(containers_path, "%s/containers", working_directory);

	d = opendir(containers_path);
	if (!d) {
		fprintf(stderr, "Failed to open directory '%s'\n", containers_path);
		goto finish;
	}

	while ((dir = readdir(d)) != NULL) {

		if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) {
			continue;
		}

		char *container_workdir_path = malloc(strlen(containers_path) + strlen(dir->d_name) + 2);
		if (!container_workdir_path) {
			fprintf(stderr, "Failed to allocate memory for container workdir path\n");
			goto finish;
		}

		strcpy(container_workdir_path, containers_path);
		strcat(container_workdir_path, "/");
		strcat(container_workdir_path, dir->d_name);

		if (rm_rf(container_workdir_path)) {
			fprintf(stderr, "Failed to remove container workdir '%s'\n", container_workdir_path);
			free(container_workdir_path);
			goto finish;
		}

		free(container_workdir_path);
	}

	ret = 0;

finish:
	if (d) {
		closedir(d);
	}

	free(containers_path);

	return ret;
}
#endif

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
		goto error;
	}

#if CONFIG_OCRE_FILESYSTEM
	delete_container_workdirs(context->working_directory);
#endif

	rc = pthread_mutex_init(&context->mutex, NULL);
	if (rc) {
		LOG_ERR("Failed to initialize context mutex: rc=%d", rc);
		goto error;
	}

	/* Initialize containers list */

	context->containers = NULL;

	return context;

error:
	free(context->working_directory);

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

	/* Remove all containers */

	LL_FOREACH(context->containers, node)
	{
		ocre_context_remove_container(context, node->container);
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
	struct container_node *node = NULL;
	char *container_workdir = NULL;
	char *image_path = NULL;
	char random_id[RANDOM_ID_LEN];
	int rc;

	rc = pthread_mutex_lock(&context->mutex);
	if (rc) {
		LOG_ERR("Failed to lock context mutex: rc=%d", rc);
		return NULL;
	}

	/* Container name checks */

	if (!container_id) {
		/* If no container ID is provided, generate a random one */

		if (make_unique_random_container_id(context, random_id, RANDOM_ID_LEN)) {
			LOG_ERR("Failed to generate random container ID");
			goto error;
		}

		computed_container_id = random_id;
	} else if (ocre_context_get_container_by_id_locked(context, container_id)) {
		LOG_ERR("Container with ID '%s' already exists", container_id);
		goto error;
	} else {
		computed_container_id = container_id;
	}

	/* Allocate the node */

	node = malloc(sizeof(struct container_node));
	if (!node) {
		LOG_ERR("Failed to allocate memory for container node");
		goto error;
	}

	memset(node, 0, sizeof(struct container_node));

	/* Build the full path to the image */

	image_path = malloc(strlen(context->working_directory) + strlen("/images/") + strlen(image) + 1);
	if (!image_path) {
		LOG_ERR("Failed to allocate memory for image path");
		goto error;
	}

	strcpy(image_path, context->working_directory);
	strcat(image_path, "/images/");
	strcat(image_path, image);

	// TODO: check if image exists

	/* Build the path to the working dir and create it */

#if CONFIG_OCRE_FILESYSTEM
	if (arguments && string_array_lookup(arguments->capabilities, "filesystem")) {
		container_workdir = malloc(strlen(context->working_directory) + strlen("/containers/") +
					   strlen(computed_container_id) + 1);
		if (!container_workdir) {
			LOG_ERR("Failed to allocate memory for working directory");
			goto error;
		}

		snprintf(container_workdir,
			 strlen(context->working_directory) + strlen("/containers/") + strlen(computed_container_id) +
				 1,
			 "%s/containers/%s", context->working_directory, computed_container_id);

		rc = mkdir(container_workdir, 0755);
		if (rc) {
			LOG_ERR("Failed to create working directory '%s': rc=%d", container_workdir, rc);
			goto error;
		}
	}
#endif

	/* Create the container */

	container = ocre_container_create(image_path, container_workdir, runtime, computed_container_id, detached,
					  arguments);
	if (!container) {
		LOG_ERR("Failed to create container %s: errno=%d", computed_container_id, errno);
		goto error;
	}

	/* Add the container to the context */

	node->container = container;
	node->working_directory = container_workdir;

	LL_APPEND(context->containers, node);

	goto success;

error:
	if (container_workdir) {
		rc = rm_rf(container_workdir);
		if (rc) {
			LOG_ERR("Failed to remove container working directory '%s': rc=%d", container_workdir, rc);
		}
	}

	free(container_workdir);

	free(node);

	/* Fall-through on error */

success:
	free(image_path);

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

#if CONFIG_OCRE_FILESYSTEM
			if (node->working_directory) {
				rc = rm_rf(node->working_directory);
				if (rc) {
					LOG_ERR("Failed to remove container working directory '%s': rc=%d",
						node->working_directory, rc);
				}
			}
#endif

			LL_DELETE(context->containers, node);

			free(node->working_directory);

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

char *ocre_context_get_working_directory(struct ocre_context *context)
{
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
