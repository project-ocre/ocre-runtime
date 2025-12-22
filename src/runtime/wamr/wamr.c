/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <ocre/runtime/vtable.h>

#include <ocre/platform/config.h>
#include <ocre/platform/file.h>
#include <ocre/platform/log.h>
#include <ocre/platform/memory.h>

#include <wasm_export.h>

#include "ocre_api/ocre_api.h"

#include "ocre_api/ocre_common.h"
#include "ocre_api/ocre_timers/ocre_timer.h"

LOG_MODULE_REGISTER(wamr_runtime, CONFIG_OCRE_LOG_LEVEL);

static wasm_shared_heap_t _shared_heap = NULL;

static void *shared_heap_buf = NULL;

struct wamr_context {
	char *buffer;
	size_t size;
	char error_buf[128];
	wasm_module_t module;
	wasm_module_inst_t module_inst;
	char **argv;
	char **envp;
	bool uses_ocre_api;
	bool uses_shared_heap;
};

static int instance_execute(void *runtime_context, pthread_cond_t *cond)
{
	struct wamr_context *context = runtime_context;

	context->module_inst =
		wasm_runtime_instantiate(context->module, 8192, 8192, context->error_buf, sizeof(context->error_buf));
	if (!context->module_inst) {
		LOG_ERR("Failed to instantiate module: %s, for context %p", context->error_buf, context);
		return -1;
	}

	if (context->uses_ocre_api) {
		ocre_module_context_t *mod = ocre_register_module(context->module_inst);
		wasm_runtime_set_custom_data(context->module_inst, mod);
	}

	if (context->uses_shared_heap) {
		if (!wasm_runtime_attach_shared_heap(context->module_inst, _shared_heap)) {
			LOG_ERR("Failed to attach shared heap");
		}
		LOG_INF("Shared heap capability enabled");
	}

	/* Clear any previous exceptions */

	wasm_runtime_clear_exception(context->module_inst);

	/* Notify the starting waiter that we are ready
	 * We should notify only after we are ready to process the kill call.
	 * In WAMR, this is managed by the exception message, so we are good if we just cleared the
	 * exception.
	 */

	int rc = pthread_cond_signal(cond);
	if (rc) {
		LOG_WRN("Failed to signal start conditional variable: rc=%d", rc);
	}

	/* Execute main function */

	const char *exception = NULL;
	if (!wasm_application_execute_main(context->module_inst, 1, context->argv)) {
		LOG_WRN("Main function returned error in context %p exception: %s", context,
			exception ? exception : "None");

		exception = wasm_runtime_get_exception(context->module_inst);
		if (exception) {
			LOG_ERR("Container %p exception: %s", context, exception);
		}
	}

	if (context->uses_ocre_api) {
		/* Cleanup module resources if using Ocre API */

		LOG_INF("Cleaning up module resources");

		ocre_cleanup_module_resources(context->module_inst);

		ocre_unregister_module(context->module_inst);
	}

	LOG_INF("Context %p completed successfully", context);

	int exit_code = wasm_runtime_get_wasi_exit_code(context->module_inst);

	wasm_runtime_deinstantiate(context->module_inst);

	context->module_inst = NULL;

	return exit_code;
}

static int instance_thread_execute(void *arg, pthread_cond_t *cond)
{
	struct wamr_context *context = arg;

	wasm_runtime_init_thread_env();

	int ret = instance_execute(context, cond);

	wasm_runtime_destroy_thread_env();

	return ret;
}

static int runtime_init(void)
{
#if defined(CONFIG_OCRE_SHARED_HEAP_BUF_VIRTUAL)

	/* Allocate memory for the shared heap */

	shared_heap_buf = user_malloc(CONFIG_OCRE_SHARED_HEAP_BUF_VIRTUAL);
	if (!shared_heap_buf) {
		LOG_ERR("Failed to allocate memory for the shared heap of size %zu",
			(size_t)CONFIG_OCRE_SHARED_HEAP_BUF_VIRTUAL);
		return -1;
	}
#elif defined(CONFIG_OCRE_SHARED_HEAP_BUF_PHYSICAL)
	shared_heap_buf = CONFIG_OCRE_SHARED_HEAP_BUF_ADDRESS;
#endif
	RuntimeInitArgs init_args;
	memset(&init_args, 0, sizeof(RuntimeInitArgs));
	init_args.mem_alloc_type = Alloc_With_Allocator;
	init_args.mem_alloc_option.allocator.malloc_func = user_malloc;
	init_args.mem_alloc_option.allocator.free_func = user_free;
	init_args.mem_alloc_option.allocator.realloc_func = user_realloc;
	// init_args.native_module_name = "env";
	// init_args.n_native_symbols = ocre_api_table_size;
	// init_args.native_symbols = ocre_api_table;

	if (!wasm_runtime_full_init(&init_args)) {
		LOG_ERR("Failed to initialize the WAMR runtime");
		goto error_sh_heap_buf;
	}

	wasm_runtime_set_log_level(CONFIG_OCRE_WAMR_LOG_LEVEL);

#ifdef CONFIG_OCRE_SHARED_HEAP
	SharedHeapInitArgs heap_init_args;
	memset(&heap_init_args, 0, sizeof(heap_init_args));
	heap_init_args.pre_allocated_addr = shared_heap_buf;
	heap_init_args.size = CONFIG_OCRE_SHARED_HEAP_BUF_SIZE;

	_shared_heap = wasm_runtime_create_shared_heap(&heap_init_args);

	if (!_shared_heap) {
		LOG_ERR("Failed to create shared heap");
		goto error_runtime;
	}
#endif

	if (!wasm_runtime_register_natives("env", ocre_api_table, ocre_api_table_size)) {
		LOG_ERR("Failed to register the API's");
		return -1;
	}

	ocre_common_init();
	ocre_timer_init();

	// TODO handle error

	return 0;

error_runtime:
	wasm_runtime_destroy();

error_sh_heap_buf:
#if defined(CONFIG_OCRE_SHARED_HEAP_BUF_VIRTUAL)
	free(shared_heap_buf);
	shared_heap_buf = NULL;
#endif

	return -1;
}

static int runtime_deinit(void)
{
	ocre_common_shutdown();

	wasm_runtime_destroy();

#ifdef CONFIG_OCRE_SHARED_HEAP_BUF_VIRTUAL
	free(shared_heap_buf);
#endif

	return 0;
}

static void *instance_create(const char *img_path, const char *workdir, const char **capabilities, const char **argv,
			     const char **envp, const char **mounts)
{
	struct wamr_context *context = NULL;
	char **dir_map_list = NULL;
	char **new_dir_map_list = NULL;
	size_t dir_map_list_len = 0;

	if (!img_path) {
		LOG_ERR("Invalid arguments");
		return NULL;
	}

	context = malloc(sizeof(struct wamr_context));
	if (!context) {
		LOG_ERR("Failed to allocate memory for context size=%zu errno=%d", sizeof(struct wamr_context), errno);
		goto error;
	}

	memset(context, 0, sizeof(struct wamr_context));

	/* For envp we can just keep a reference
	 * as the container is guaranteed to only free it after our destruction
	 */

	context->envp = (char **)envp;

	int envn = 0;
	while (context->envp && context->envp[envn]) {
		envn++;
	}

	/* We need to insert argv[0]. We can keep a shallow copy, because
	 * the container is guaranteed to only free it after us
	 */

	int argc = 0;
	while (argv && argv[argc]) {
		argc++;
	}

	/* 2 more: app name and NULL */

	context->argv = malloc(sizeof(char *) * (argc + 2));
	if (!context->argv) {
		LOG_ERR("Failed to allocate memory for argv");
		goto error;
	}

	memset(context->argv, 0, sizeof(char *) * (argc + 2));

	context->argv[0] = strdup(img_path);
	if (!context->argv[0]) {
		goto error;
	}

	int i;
	for (i = 0; i < argc; i++) {
		context->argv[i + 1] = (char *)argv[i];
		if (!context->argv[i + 1]) {
			goto error;
		}
	}

	context->argv[i + 1] = NULL;

	if (context->buffer) {
		LOG_WRN("Buffer already allocated. Possible memory leak!");
	}

	/* Memory-map file */

	context->buffer = ocre_load_file(img_path, &context->size);
	if (!context->buffer) {
		LOG_ERR("Failed to load wasm program into buffer errno=%d", errno);
		goto error;
	}

	LOG_INF("Buffer loaded successfully: %p", context->buffer);

	context->module = wasm_runtime_load((uint8_t *)context->buffer, context->size, context->error_buf,
					    sizeof(context->error_buf));
	if (!context->module) {
		LOG_ERR("Failed to load module: %s", context->error_buf);
		goto error;
	}

	/* Process capabilities */

	for (const char **cap = capabilities; cap && *cap; cap++) {
		if (!strcmp(*cap, "ocre:shared_heap")) {
			context->uses_shared_heap = true;
		} else if (!strcmp(*cap, "ocre:api")) {
			context->uses_ocre_api = true;
		}
#if CONFIG_OCRE_NETWORKING
		else if (!strcmp(*cap, "networking")) {
			const char *addr_pool[] = {
				"0.0.0.0/0",
			};

			wasm_runtime_set_wasi_addr_pool(context->module, addr_pool,
							sizeof(addr_pool) / sizeof(addr_pool[0]));

			const char *ns_lookup_pool[] = {"*"};

			wasm_runtime_set_wasi_ns_lookup_pool(context->module, ns_lookup_pool,
							     sizeof(ns_lookup_pool) / sizeof(ns_lookup_pool[0]));

			LOG_INF("Network capability enabled");
		}
#endif
#if CONFIG_OCRE_FILESYSTEM
		else if (!strcmp(*cap, "filesystem") && workdir) {
			dir_map_list = malloc((dir_map_list_len + 2) * sizeof(char *));
			if (!dir_map_list) {
				LOG_ERR("Failed to allocate memory for dir_map_list");
				goto error;
			}

			memset(dir_map_list, 0, sizeof(char *));

			dir_map_list[dir_map_list_len] = malloc(strlen("/::") + strlen(workdir) + 1);
			if (!dir_map_list[dir_map_list_len]) {
				LOG_ERR("Failed to allocate memory for dir_map_list[0]");
				free(dir_map_list);
				goto error;
			}

			sprintf(dir_map_list[dir_map_list_len], "/::%s", workdir);

			dir_map_list_len++;

			dir_map_list[dir_map_list_len] = NULL;

			LOG_INF("Filesystem capability enabled");
		}
#endif
	}

	/* Add the mounts to the directory map list */

	for (const char **mount = mounts; mount && *mount; mount++) {
		/* Need to insert the extra ':' */

		new_dir_map_list = realloc(dir_map_list, (dir_map_list_len + 2) * sizeof(char *));
		if (!new_dir_map_list) {
			LOG_ERR("Failed to allocate memory for dir_map_list");
			goto error;
		}

		dir_map_list = new_dir_map_list;

		dir_map_list[dir_map_list_len] = malloc(strlen(*mount) + 2);
		if (!dir_map_list[dir_map_list_len]) {
			LOG_ERR("Failed to allocate memory for dir_map_list[%zu]", dir_map_list_len);
			goto error;
		}

		const char *src_colon = strchr(*mount, ':');
		if (!src_colon) {
			LOG_ERR("Invalid mount format: %s", *mount);
			goto error;
		}

		strcpy(dir_map_list[dir_map_list_len], *mount);

		char *dst_colon = strchr(dir_map_list[dir_map_list_len], ':');
		if (!dst_colon) {
			LOG_ERR("Invalid mount format: %s", *mount);
			goto error;
		}

		sprintf(dst_colon + 1, ":%s", src_colon + 1);

		LOG_INF("Enabled mount: %s", dir_map_list[dir_map_list_len]);

		dir_map_list_len++;

		/* Add the NULL */

		dir_map_list[dir_map_list_len] = NULL;
	}

	wasm_runtime_set_wasi_args(context->module, NULL, 0, (const char **)dir_map_list, dir_map_list_len, envp, envn,
				   context->argv, argc + 1);

	for (char **dir_map = dir_map_list; dir_map && *dir_map; dir_map++) {
		free(*dir_map);
	}

	free(dir_map_list);

	return context;

error_module:
	wasm_runtime_unload(context->module);

error:
	for (char **dir_map = dir_map_list; dir_map && *dir_map; dir_map++) {
		free(*dir_map);
	}

	free(dir_map_list);

	if (context) {
		if (context->module) {
			wasm_runtime_unload(context->module);
		}

		if (context->buffer) {
			ocre_unload_file(context->buffer, context->size);
			context->buffer = NULL;
		}

		/* Only free what we allocated */

		if (context->argv) {
			free(context->argv[0]);
		}

		free(context->argv);
	}

	free(context);

	return NULL;
}

static int instance_kill(void *runtime_context)
{
	struct wamr_context *context = runtime_context;

	if (!context) {
		return -1;
	}

	wasm_runtime_terminate(context->module_inst);

	return 0;
}

static int instance_destroy(void *runtime_context)
{
	struct wamr_context *context = runtime_context;

	if (!context) {
		return -1;
	}

	wasm_runtime_unload(context->module);

	if (ocre_unload_file(context->buffer, context->size)) {
		LOG_ERR("Failed to unload file");
	}
	context->buffer = NULL;

	free(context->argv[0]);
	free(context->argv);

	free(context);

	return 0;
}

const struct ocre_runtime_vtable wamr_vtable = {
	.runtime_name = "wamr",
	.init = runtime_init,
	.deinit = runtime_deinit,
	.create = instance_create,
	.destroy = instance_destroy,
	.thread_execute = instance_thread_execute,
	.kill = instance_kill,
};
