#include <string.h>
#include <stdlib.h>
#include <strings.h>

#include <ocre/runtime/wamr/wamr.h>

#include <ocre/ocre.h>

#include <ocre/platform/log.h>

#include <uthash/utlist.h>

#include <ocre/build_info.h>

#include "commit_id.h"
#include "version.h"

LOG_MODULE_REGISTER(ocre, CONFIG_OCRE_LOG_LEVEL);

/* Constant build information */

const struct ocre_config ocre_build_configuration = {
	.build_info = OCRE_BUILD_HOST_INFO,
	.version = OCRE_VERSION_STRING,
	.commit_id = GIT_COMMIT_ID,
};

/* List of runtimes */

struct runtime_node {
	const struct ocre_runtime_vtable *runtime;
	struct runtime_node *next;
};

static struct runtime_node *runtimes = NULL;

int ocre_initialize_with_runtimes(const struct ocre_runtime_vtable *const vtable[])
{
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

int ocre_initialize(void)
{
	LOG_INF("Initializing OCRE %s", ocre_build_configuration.version);
	LOG_INF("Build Commit ID: %s", ocre_build_configuration.commit_id);
	LOG_INF("Build Host: %s", ocre_build_configuration.build_info);

	return ocre_initialize_with_runtimes((const struct ocre_runtime_vtable *const[]){NULL});
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
	struct runtime_node *elt, *tmp;

	LL_FOREACH_SAFE(runtimes, elt, tmp)
	{
		if (elt->runtime->deinit && elt->runtime->deinit()) {
			LOG_INF("Failed to deinitialize '%s'", elt->runtime->runtime_name);
		}

		LL_DELETE(runtimes, elt);

		free(elt);
	}
}
