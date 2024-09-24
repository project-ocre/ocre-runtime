/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#include "cs_sm.h"
#include "cs_sm_impl.h"
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>

// Internal Data structures for runtime
static char wamr_heap_buf[CONFIG_OCRE_WAMR_HEAP_BUFFER_SIZE] = {0};

static int load_binary_to_buffer_fs(ocre_cs_ctx *ctx, int container_id, ocre_container_data_t *container_data) {
    int ret = 0;
    static char filepath[FILE_PATH_MAX];
    struct fs_file_t app_file;
    struct fs_dirent entry;

    snprintf(filepath, sizeof(filepath), "/lfs/ocre/images/%s.bin", container_data->sha256);

    ret = fs_stat(filepath, &entry);
    if (ret < 0) {
        LOG_ERR("Failed to get file status for %s: %d", filepath, ret);
        return ret;
    }

    ctx->containers[container_id].ocre_runtime_arguments.size = entry.size;
    ctx->containers[container_id].ocre_runtime_arguments.buffer = malloc(entry.size * sizeof(char));

    if (ctx->containers[container_id].ocre_runtime_arguments.buffer == NULL) {
        LOG_ERR("Failed to allocate memory for container binary.");
        return -ENOMEM;
    }

    LOG_INF("File path: %s, size: %d", filepath, entry.size);
    fs_file_t_init(&app_file);

    ret = fs_open(&app_file, filepath, FS_O_READ);

    if (ret < 0) {
        LOG_ERR("Failed to open file %s: %d", filepath, ret);
        return ret;
    }

    ret = fs_read(&app_file, ctx->containers[container_id].ocre_runtime_arguments.buffer,
                  ctx->containers[container_id].ocre_runtime_arguments.size);
    if (ret < 0) {
        LOG_ERR("Failed to read file %s: %d", filepath, ret);
        fs_close(&app_file);
        return ret;
    }

    ret = fs_close(&app_file);

    if (ret < 0) {
        LOG_ERR("Failed to close file %s: %d", filepath, ret);
        return ret;
    }

    return 0;
}

int CS_ctx_init(ocre_cs_ctx *ctx) {
    ctx->current_container_id = 0;
    ctx->download_count = 0;

    for (int i = 0; i < MAX_CONTAINERS; i++) {
        ctx->containers[i].container_runtime_status = CONTAINER_STATUS_UNKNOWN;
        ctx->containers[i].ocre_container_data.heap_size = 2048;
        ctx->containers[i].ocre_container_data.stack_size = 2048;
        memset(ctx->containers[i].ocre_container_data.name, 0, sizeof(ctx->containers[i].ocre_container_data.name));
        memset(ctx->containers[i].ocre_container_data.sha256, 0, sizeof(ctx->containers[i].ocre_container_data.sha256));
        ctx->containers[i].ocre_container_data.timers = 0;
        ctx->containers[i].ocre_container_data.watchdog_interval = 0;
    }
    return 0;
}

// Supervisor functions
ocre_container_runtime_status_t CS_runtime_init(ocre_cs_ctx *ctx, ocre_container_init_arguments_t *args) {
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = wamr_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(wamr_heap_buf);
    /* configure the native functions being exported to WASM app */
    init_args.native_module_name = "env";
    init_args.n_native_symbols = ocre_api_table_size;
    init_args.native_symbols = ocre_api_table;

    if (!wasm_runtime_full_init(&init_args)) {
        LOG_ERR("Failed to initialize the WASM runtime");
        return RUNTIME_STATUS_ERROR;
    }
}

ocre_container_runtime_status_t CS_runtime_destroy(void) {
    wasm_runtime_destroy();
    destroy_ocre_cs_thread();
    return RUNTIME_STATUS_DESTROYED;
}

// Container functions

ocre_container_status_t CS_create_container(ocre_cs_ctx *ctx, int container_id) {
    ctx->containers[container_id].ocre_runtime_arguments.stack_size =
            ctx->containers[container_id].ocre_container_data.stack_size;
    ctx->containers[container_id].ocre_runtime_arguments.heap_size =
            ctx->containers[container_id].ocre_container_data.heap_size;
    ctx->containers[container_id].ocre_runtime_arguments.stack_size = 2048;
    ctx->containers[container_id].ocre_runtime_arguments.heap_size = 2048;

    int ret = load_binary_to_buffer_fs(ctx, container_id, &ctx->containers[container_id].ocre_container_data);

    if (ret < 0) {
        LOG_ERR("Failed to load binary to buffer: %d", ret);
        return CONTAINER_STATUS_ERROR;
    }

    LOG_INF("Loaded binary to buffer for container %d", container_id);
    
    if (ctx->containers[container_id].ocre_container_data.watchdog_interval > 0) {
        ocre_healthcheck_init(&ctx->containers[container_id].ocre_container_data.WDT,
                              ctx->containers[container_id].ocre_container_data.watchdog_interval);
    }

    ctx->containers[container_id].ocre_runtime_arguments.module =
            wasm_runtime_load(ctx->containers[container_id].ocre_runtime_arguments.buffer,
                              ctx->containers[container_id].ocre_runtime_arguments.size,
                              ctx->containers[container_id].ocre_runtime_arguments.error_buf,

                              sizeof(ctx->containers[container_id].ocre_runtime_arguments.error_buf));
    if (!ctx->containers[container_id].ocre_runtime_arguments.module) {
        LOG_ERR("Failed to load WASM module: %s", ctx->containers[container_id].ocre_runtime_arguments.error_buf);
        return CONTAINER_STATUS_ERROR;
    }

    ctx->containers[container_id].ocre_runtime_arguments.module_inst =
            wasm_runtime_instantiate(ctx->containers[container_id].ocre_runtime_arguments.module,
                                     ctx->containers[container_id].ocre_runtime_arguments.stack_size,
                                     ctx->containers[container_id].ocre_runtime_arguments.heap_size,
                                     ctx->containers[container_id].ocre_runtime_arguments.error_buf,
                                     sizeof(ctx->containers[container_id].ocre_runtime_arguments.error_buf));

    if (!ctx->containers[container_id].ocre_runtime_arguments.module_inst) {
        LOG_ERR("Failed to instantiate WASM module: %s",
                ctx->containers[container_id].ocre_runtime_arguments.error_buf);
        return CONTAINER_STATUS_ERROR;
    }

    ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_CREATED;

    return ctx->containers[container_id].container_runtime_status;
}

ocre_container_status_t CS_run_container(ocre_cs_ctx *ctx, int container_id) {

    if (ctx->containers[container_id].container_runtime_status == CONTAINER_STATUS_CREATED) {
        uint32_t argv[2];
        argv[0] = 8;

        /* Create an instance of the WASM module (WASM linear memory is ready) */
        /* Lookup a WASM function by its name */
        ctx->containers[container_id].ocre_runtime_arguments.func = wasm_runtime_lookup_function(
                ctx->containers[container_id].ocre_runtime_arguments.module_inst, "on_init");

        if (NULL == ctx->containers[container_id].ocre_runtime_arguments.func) {
            LOG_ERR("ERROR lookup function: ");
        }

        /* creat an execution environment to execute the WASM functions */
        ctx->containers[container_id].ocre_runtime_arguments.exec_env =
                wasm_runtime_create_exec_env(ctx->containers[container_id].ocre_runtime_arguments.module_inst,
                                             ctx->containers[container_id].ocre_runtime_arguments.stack_size);

        if (NULL == ctx->containers[container_id].ocre_runtime_arguments.exec_env) {
            LOG_ERR("ERROR creating executive environment: ");
        }

        /* call the WASM function */
        if (!wasm_runtime_call_wasm(ctx->containers[container_id].ocre_runtime_arguments.exec_env,
                                    ctx->containers[container_id].ocre_runtime_arguments.func, 1, argv)) {
            LOG_ERR("ERROR calling main:");
        }

        ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_RUNNING;

        return CONTAINER_STATUS_RUNNING;
    } else {
        LOG_ERR("Container (ID: %d), does not exist to run it", container_id);

        ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_UNKNOWN;

        return CONTAINER_STATUS_ERROR;
    }
}

ocre_container_status_t CS_get_container_status(ocre_cs_ctx *ctx, int container_id) {

    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    return ctx->containers[container_id].container_runtime_status;
}

ocre_container_status_t CS_stop_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback) {

    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    wasm_runtime_deinstantiate(ctx->containers[container_id].ocre_runtime_arguments.module_inst);
    ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_STOPPED;

    return CONTAINER_STATUS_STOPPED;
}

ocre_container_status_t CS_destroy_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback) {

    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    wasm_runtime_unload(ctx->containers[container_id].ocre_runtime_arguments.module);
    ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_DESTROYED;
    ctx->current_container_id--;

    return CONTAINER_STATUS_DESTROYED;
}

ocre_container_status_t CS_restart_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback) {

    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    CS_stop_container(ctx, container_id, callback);
    CS_run_container(ctx, container_id);
    ocre_healthcheck_reinit(&ctx->containers[container_id].ocre_container_data.WDT);
    ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_RUNNING;
    
    return CONTAINER_STATUS_RUNNING;
}
