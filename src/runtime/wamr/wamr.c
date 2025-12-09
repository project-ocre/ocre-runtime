#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

LOG_MODULE_REGISTER(wamr_runtime, 3);

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
};

static int instance_execute(void *runtime_context)
{
	struct wamr_context *context = runtime_context;
	wasm_module_inst_t module_inst = context->module_inst;

	/* Clear any previous exceptions */

	wasm_runtime_clear_exception(module_inst);

	/* Execute main function */

	const char *exception = NULL;
	if (!wasm_application_execute_main(module_inst, 1, context->argv)) {
		LOG_WRN("Main function returned error in context %p exception: %s", context,
			exception ? exception : "None");

		exception = wasm_runtime_get_exception(module_inst);
		if (exception) {
			LOG_ERR("Container %p exception: %s", context, exception);
		}
	}

	if (context->uses_ocre_api) {
		/* Cleanup module resources if using OCRE API */

		LOG_INF("Cleaning up module resources");

		ocre_cleanup_module_resources(module_inst);
	}

	LOG_INF("Context %p completed successfully", context);

	return wasm_runtime_get_wasi_exit_code(context->module_inst);
}

static int instance_thread_execute(void *arg)
{
	struct wamr_context *context = arg;

	wasm_runtime_init_thread_env();

	int ret = instance_execute(context);

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

	wasm_runtime_set_log_level(WASM_LOG_LEVEL_VERBOSE);

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

static void *instance_create(const char *img_path, const char *workdir, size_t stack_size, size_t heap_size,
			     const char **capabilities, const char **argv, const char **envp, const char **mounts)
{
	if (!img_path) {
		LOG_ERR("Invalid arguments");
		return NULL;
	}

	struct wamr_context *context = malloc(sizeof(struct wamr_context));
	if (!context) {
		LOG_ERR("Failed to allocate memory for context size=%zu errno=%d", sizeof(struct wamr_context), errno);
		return NULL;
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
		goto error_context;
	}

	memset(context->argv, 0, sizeof(char *) * (argc + 2));

	context->argv[0] = strdup(img_path);
	if (!context->argv[0]) {
		goto error_argv;
	}

	int i;
	for (i = 0; i < argc; i++) {
		context->argv[i + 1] = strdup(argv[i]);
		if (!context->argv[i + 1]) {
			goto error_argv;
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
		goto error_argv;
	}

	LOG_INF("Buffer loaded successfully: %p", context->buffer);

	context->module = wasm_runtime_load((uint8_t *)context->buffer, context->size, context->error_buf,
					    sizeof(context->error_buf));
	if (!context->module) {
		LOG_ERR("Failed to load module: %s", context->error_buf);
		goto error_buffer;
	}

	const char **dir_map_list = NULL;
	size_t dir_map_list_len = 0;

	/* Process capabilities */

	for (const char **cap = capabilities; cap && *cap; cap++) {
		if (!strcmp(*cap, "networking")) {
			const char *addr_pool[] = {
				"0.0.0.0/0",
			};

			wasm_runtime_set_wasi_addr_pool(context->module, addr_pool,
							sizeof(addr_pool) / sizeof(addr_pool[0]));

			const char *ns_lookup_pool[] = {"*"};

			wasm_runtime_set_wasi_ns_lookup_pool(context->module, ns_lookup_pool,
							     sizeof(ns_lookup_pool) / sizeof(ns_lookup_pool[0]));

			LOG_INF("Network capability enabled");
		} else if (!strcmp(*cap, "filesystem")) {
#ifdef PICO_RP2350
			static const char *dir_map[] = {"/::/lfs/cfs"}; // TODO: fix this
#else
			static const char *dir_map[] = {"/::/"}; // TODO: fix this
#endif
			dir_map_list = dir_map;
			dir_map_list_len = sizeof(dir_map) / sizeof(dir_map[0]);
			LOG_INF("Filesystem capability enabled");
		}
	}

	wasm_runtime_set_wasi_args(context->module, NULL, 0, dir_map_list, dir_map_list_len, envp, envn, context->argv,
				   argc + 1);

	context->module_inst = wasm_runtime_instantiate(context->module, stack_size, heap_size, context->error_buf,
							sizeof(context->error_buf));
	if (!context->module_inst) {
		LOG_ERR("Failed to instantiate module: %s, for context %p", context->error_buf, context);
		goto error_module;
	}

	for (const char **cap = capabilities; cap && *cap; cap++) {
		if (!strcmp(*cap, "networking") || !strcmp(*cap, "filesystem")) {
			// already set up
		} else if (!strcmp(*cap, "ocre:api")) {
			context->uses_ocre_api = true;
			ocre_module_context_t *mod = ocre_register_module(context->module_inst);
			wasm_runtime_set_custom_data(context->module_inst, mod);
		} else if (!strcmp(*cap, "ocre:shared_heap")) {
			if (!wasm_runtime_attach_shared_heap(context->module_inst, _shared_heap)) {
				LOG_ERR("Failed to attach shared heap");
				goto error_instance;
			}
			LOG_INF("Shared heap capability enabled");
		} else {
			LOG_WRN("Capability '%s' not supported by runtime", *cap);
		}
	}

	return context;

error_instance:
	wasm_runtime_deinstantiate(context->module_inst);

error_module:
	wasm_runtime_unload(context->module);

error_buffer:
	ocre_unload_file(context->buffer, context->size);
	context->buffer = NULL;

error_argv:
	/* Only free what we allocated */

	free(context->argv[0]);
	free(context->argv);

error_context:
	free(context);

	return NULL;
}

static int instance_stop(void *runtime_context)
{
	struct wamr_context *context = runtime_context;

	if (!context) {
		return -1;
	}

	// TODO

	return -1;
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

static int instance_pause(void *runtime_context)
{
	struct wamr_context *context = runtime_context;

	if (!context) {
		return -1;
	}

	// TODO

	return -1;
}

static int instance_unpause(void *runtime_context)
{
	struct wamr_context *context = runtime_context;

	if (!context) {
		return -1;
	}

	// TODO

	return -1;
}

static int instance_destroy(void *runtime_context)
{
	struct wamr_context *context = runtime_context;

	if (!context) {
		return -1;
	}

	if (context->uses_ocre_api) {
		ocre_unregister_module(context->module_inst);
	}

	wasm_runtime_deinstantiate(context->module_inst);

	context->module_inst = NULL;

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
	.stop = instance_stop,
	.pause = instance_pause,
	.unpause = instance_unpause,
};
