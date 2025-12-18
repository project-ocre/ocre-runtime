#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>

#include <ocre/ocre.h>
#include <ocre/platform/log.h>
#include <ocre/build_info.h>
#include <ocre/runtime/wamr/wamr.h>

#include "context.h"
#include "util/rm_rf.h"
#include "uthash/utlist.h"

#include "commit_id.h"
#include "version.h"

LOG_MODULE_REGISTER(ocre, CONFIG_OCRE_LOG_LEVEL);

/* Constant build information */

const struct ocre_config ocre_build_configuration = {
	.build_info = OCRE_BUILD_HOST_INFO,
	.version = OCRE_VERSION_STRING,
	.commit_id = GIT_COMMIT_ID,
	.build_date = OCRE_BUILD_DATE,
};

/* List of runtimes */

struct runtime_node {
	const struct ocre_runtime_vtable *runtime;
	struct runtime_node *next;
};

struct context_node {
	struct ocre_context *context;
	char *working_directory;
	struct context_node *next;
};

static struct runtime_node *runtimes = NULL;
static struct context_node *contexts = NULL;
static pthread_mutex_t contexts_mutex = PTHREAD_MUTEX_INITIALIZER;

static int create_dir_if_not_exists(const char *path)
{
	int rc;
	struct stat st;

	rc = stat(path, &st);
	if (rc && errno == ENOENT) {
		LOG_INF("Directory '%s' does not exist, creating...", path);
		rc = mkdir(path, 0755);
		if (rc) {
			LOG_ERR("Failed to create directory '%s': errno=%d", path, errno);
			return -1;
		}
	} else if (!rc && !S_ISDIR(st.st_mode)) {
		LOG_ERR("Path '%s' is not a directory", path);
		return -1;
	} else if (!rc && S_ISDIR(st.st_mode)) {
		LOG_INF("Directory '%s' already exists", path);
	}

	return 0;
}

static int populate_context_workdir(const char *working_directory)
{
	int rc = -1;
	char *containers_path = NULL;
	char *images_path = NULL;

	rc = create_dir_if_not_exists(working_directory);
	if (rc) {
		LOG_ERR("Failed to create Ocre working directory '%s': errno=%d", working_directory, errno);
		return -1;
	}

	containers_path = malloc(strlen(working_directory) + strlen("/containers") + 1);
	if (!containers_path) {
		LOG_ERR("Failed to allocate memory for container workdir path");
		goto finish;
	}

	sprintf(containers_path, "%s/containers", working_directory);

	rc = create_dir_if_not_exists(containers_path);
	if (rc) {
		LOG_ERR("Failed to create Ocre containers directory '%s': errno=%d", containers_path, errno);
		goto finish;
	}

	images_path = malloc(strlen(working_directory) + strlen("/images") + 1);
	if (!images_path) {
		LOG_ERR("Failed to allocate memory for container workdir path");
		goto finish;
	}

	sprintf(images_path, "%s/images", working_directory);

	rc = create_dir_if_not_exists(images_path);
	if (rc) {
		LOG_ERR("Failed to create Ocre images directory '%s': errno=%d", images_path, errno);
		goto finish;
	}

finish:
	free(containers_path);
	free(images_path);

	return rc;
}

#if CONFIG_OCRE_FILESYSTEM
static int delete_container_workdirs(const char *working_directory)
{
	int ret = -1;
	DIR *d = NULL;
	const struct dirent *dir = NULL;

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

static int ocre_destroy_context_locked(struct ocre_context *context)
{
	int rc;
	struct context_node *node = NULL;
	struct context_node *elt = NULL;

	LL_FOREACH_SAFE(contexts, node, elt)
	{
		if (node->context == context) {
			rc = ocre_context_destroy(context);
			if (rc) {
				LOG_ERR("Failed to destroy context: rc=%d", rc);
				return rc;
			}

			LL_DELETE(contexts, node);

			free(node->working_directory);
			free(node);

			return 0;
		}
	}
}

struct ocre_context *ocre_create_context(const char *workdir)
{
	int rc;

	if (!workdir) {
		workdir = CONFIG_OCRE_DEFAULT_WORKING_DIRECTORY;
		LOG_INF("Using default working directory: %s", workdir);
	}

	// TODO check if working directory is already taken

	struct context_node *node = malloc(sizeof(struct context_node));
	if (!node) {
		LOG_ERR("Failed to allocate memory for context node: errno=%d", errno);
		return NULL;
	}

	memset(node, 0, sizeof(struct context_node));

	node->working_directory = strdup(workdir);
	if (!node->working_directory) {
		LOG_ERR("Failed to allocate memory for working directory: errno=%d", errno);
		goto error;
	}

	/* Initialize working directory */

	rc = populate_context_workdir(node->working_directory);
	if (rc) {
		LOG_ERR("Failed to populate Ocre working directory: rc=%d", rc);
		goto error;
	}

#if CONFIG_OCRE_FILESYSTEM
	delete_container_workdirs(node->working_directory);
#endif

    node->context = ocre_context_create(node->working_directory);
    if (!node->context) {
        LOG_ERR("Failed to create Ocre context");
        goto error;
    }

	return node->context;

error:
	free(node->working_directory);

	free(node);

	return NULL;
};

int ocre_destroy_context(struct ocre_context *context)
{
    int rc = -1;

	if (!context) {
		LOG_ERR("Invalid context");
		return -1;
	}

	rc = pthread_mutex_lock(&contexts_mutex);
	if (rc) {
		LOG_ERR("Failed to lock mutex: rc=%d", rc);
		return -1;
	}

	rc = ocre_destroy_context_locked(context);

	rc = pthread_mutex_unlock(&contexts_mutex);
	if (rc) {
		LOG_ERR("Failed to unlock mutex: rc=%d", rc);
		return -1;
	}

	return rc;
};

int ocre_initialize(const struct ocre_runtime_vtable *const vtable[])
{
	LOG_INF("Ocre version %s", ocre_build_configuration.version);
	LOG_INF("Commit ID: %s", ocre_build_configuration.commit_id);
	LOG_INF("Build information: %s", ocre_build_configuration.build_info);
	LOG_INF("Build date: %s", ocre_build_configuration.build_date);

	/* Reinitialize the static variables */

	runtimes = NULL;
	contexts = NULL;

	int rc = pthread_mutex_init(&contexts_mutex, NULL);
	if (rc) {
		LOG_ERR("Failed to initialize Ocre mutex: rc=%d", rc);
		return -1;
	}

	/* Add WAMR runtime to the list */

	struct runtime_node *wamr = malloc(sizeof(struct runtime_node));
	if (!wamr) {
		LOG_ERR("Failed to allocate memory for WAMR node");
		return -1;
	}

	memset(wamr, 0, sizeof(struct runtime_node));
	wamr->runtime = &wamr_vtable;

	LL_APPEND(runtimes, wamr);

	/* Add extra runtimes */

	if (vtable) {
		for (int i = 0; vtable[i] != NULL; i++) {
			struct runtime_node *add = malloc(sizeof(struct runtime_node));
			if (!add) {
				LOG_ERR("Failed to allocate memory for runtime node");
				return -1;
			}

			memset(add, 0, sizeof(struct runtime_node));
			add->runtime = (const struct ocre_runtime_vtable *)&vtable[i];

			LL_APPEND(runtimes, add);

			LOG_INF("Registered '%s'", vtable[i]->runtime_name);
		}
	}

	/* Initialize runtimes in the list */

	struct runtime_node *elt;

	LL_FOREACH(runtimes, elt)
	{
		if (elt->runtime->init && elt->runtime->init()) {
			LOG_INF("Failed to initialize '%s'", elt->runtime->runtime_name);
			return -1;
		}

		LOG_INF("Initialized '%s'", elt->runtime->runtime_name);
	}

	return 0;
}

const struct ocre_runtime_vtable *ocre_get_runtime(const char *name)
{
	if (!name) {
		return NULL;
	}

	struct runtime_node *elt;

	LL_FOREACH(runtimes, elt)
	{
		if (!strcmp(elt->runtime->runtime_name, name)) {
			return elt->runtime;
		}
	}

	return NULL;
}

void ocre_deinitialize(void)
{
    struct context_node *c_node;
    struct runtime_node *r_elt, *r_tmp;

   	LL_FOREACH(contexts, c_node)
	{
		if (ocre_destroy_context_locked(c_node->context)) {
			LOG_ERR("Failed to destroy context %p", c_node->context);
		}
	}

	LL_FOREACH_SAFE(runtimes, r_elt, r_tmp)
	{
		if (r_elt->runtime->deinit && r_elt->runtime->deinit()) {
			LOG_INF("Failed to deinitialize '%s'", r_elt->runtime->runtime_name);
		}

		LL_DELETE(runtimes, r_elt);

		free(r_elt);
	}

	int rc = pthread_mutex_destroy(&contexts_mutex);
	if (rc) {
		LOG_ERR("Failed to destroy context mutex: rc=%d", rc);
	}
}

int ocre_is_valid_id(const char *id)
{
	/* Cannot be NULL */

	if (!id) {
		return 0;
	}

	/* Cannot be empty or start with a dot '.' */

	if (id[0] == '\0' || id[0] == '.') {
		return 0;
	}

	/* Can only contain alphanumeric characters, dots, underscores, and hyphens */

	for (size_t i = 0; i < strlen(id); i++) {
		if ((isalnum(id[i]) && islower(id[i])) || id[i] == '.' || id[i] == '_' || id[i] == '-') {
			continue;
		}

		return 0;
	}

	return 1;
}
