/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <zephyr/logging/log.h>

#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

#include "../api/ocre_api.h"

#include <ocre/ocre.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#include <ocre/fs/fs.h>
#include <ocre/sm/sm.h>
#include "ocre/ocre_container_runtime/ocre_container_runtime.h"
#include "ocre/components/container_supervisor/cs_sm.h"
#include "ocre/container_healthcheck/ocre_container_healthcheck.h"

// Data structures shared with cs_sm
extern struct ocre_container_t containers[MAX_CONTAINERS];
extern int current_container_id;
extern int download_count;

// Internal Data structures for runtime
static char filepath[FILE_PATH_MAX];
static char wamr_heap_buf[CONFIG_OCRE_WAMR_HEAP_BUFFER_SIZE] = {0};

static int load_binary_to_buffer_fs(struct ocre_cs_ctx *ctx, int container_id, ocre_container_data_t *container_data) {
    int ret = 0;
    struct fs_file_t app_file;
    struct fs_dirent entry;

    snprintf(filepath, sizeof(filepath), "/lfs/ocre/images/%s.bin", container_data->sha256);

    ret = fs_stat(filepath, &entry);
    if (0 > ret) {
        LOG_ERR("Fail with file( %s ): %d", filepath, ret);
    }

    containers[container_id].ocre_runtime_arguments.size = entry.size;
    containers[container_id].ocre_runtime_arguments.buffer = malloc(entry.size * sizeof(char));
    LOG_INF("file path( %s ), size (%d): ", filepath, entry.size);
    fs_file_t_init(&app_file);

    ret = fs_open(&app_file, filepath, FS_O_READ);
    if (0 > ret) {
        LOG_ERR("Fail open file( %s ): %d", filepath, ret);
        return ret;
    }

    ret = fs_read(&app_file, containers[container_id].ocre_runtime_arguments.buffer,
                  containers[container_id].ocre_runtime_arguments.size);
    if (0 > ret) {
        return ret;
    }

    ret = fs_close(&app_file);
    if (0 > ret) {
        return ret;
    }

    return ret;
}

ocre_container_status_t ocre_container_runtime_init(ocre_container_init_arguments_t *args) {
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = wamr_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(wamr_heap_buf);
    /* configure the native functions being exported to WASM app */
    init_args.native_module_name = "env";
    init_args.n_native_symbols = ocre_api_table_size;
    init_args.native_symbols = ocre_api_table;
    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        LOG_ERR("Failed to initialize the WASM runtime");
        return;
    }
    start_ocre_cs_thread();
    return RUNTIME_STATUS_INITIALIZED;
}

ocre_container_status_t ocre_container_runtime_destroy(void) {
    destroy_ocre_cs_thread();
    return RUNTIME_STATUS_DESTROYED;
}

ocre_container_status_t ocre_container_runtime_create_container(ocre_cs_ctx *ctx, int container_id) {
    // setting default values
    containers[container_id].ocre_runtime_arguments.stack_size = 8092;
    containers[container_id].ocre_runtime_arguments.heap_size = 8092;

    int ret = load_binary_to_buffer_fs(ctx, container_id, &containers[container_id].ocre_container_data);
    if (0 > ret) {
        LOG_ERR("Failed to load binary to buffer LOL");
    }
    LOG_INF("Loaded binary to buffer (%d)", ret);

    // take the info about the app heap, timers, watchdogs needed for run
    if (containers[container_id].ocre_container_data.heap_size > 0) {
        containers[container_id].ocre_runtime_arguments.stack_size =
                containers[container_id].ocre_container_data.heap_size;
        containers[container_id].ocre_runtime_arguments.heap_size =
                containers[container_id].ocre_container_data.heap_size;
    }
  
    if (containers[container_id].ocre_container_data.timers > 0) {
        // the timers API is in ocre_timer.h
    }
  
    if (containers[container_id].ocre_container_data.watchdog_interval > 0) {
        ocre_healthcheck_init(&containers[container_id].ocre_container_data.WDT,
                              containers[container_id].ocre_container_data.watchdog_interval);
    }
  
    /* parse the WASM file from buffer and create a WASM module */
    containers[container_id].ocre_runtime_arguments.module =
            wasm_runtime_load(containers[container_id].ocre_runtime_arguments.buffer,
                              containers[container_id].ocre_runtime_arguments.size,
                              containers[container_id].ocre_runtime_arguments.error_buf,
                              sizeof(containers[container_id].ocre_runtime_arguments.error_buf));
    if (!containers[container_id].ocre_runtime_arguments.module) {
        LOG_ERR("ERROR load: %s", containers[container_id].ocre_runtime_arguments.error_buf);
    }
    /* create an instance of the WASM module (WASM linear memory is ready) */
    containers[container_id].ocre_runtime_arguments.module_inst =
            wasm_runtime_instantiate(containers[container_id].ocre_runtime_arguments.module,
                                     containers[container_id].ocre_runtime_arguments.stack_size,
                                     containers[container_id].ocre_runtime_arguments.heap_size,
                                     containers[container_id].ocre_runtime_arguments.error_buf,
                                     sizeof(containers[container_id].ocre_runtime_arguments.error_buf));
    if (!containers[container_id].ocre_runtime_arguments.module_inst) {
        LOG_ERR("ERROR instantiate: %s", containers[container_id].ocre_runtime_arguments.error_buf);
    }

    containers[container_id].container_runtime_status = CONTAINER_STATUS_CREATED;
    return CONTAINER_STATUS_CREATED;
}

ocre_container_status_t ocre_container_runtime_run_container(ocre_cs_ctx *ctx, int container_id) {
    uint32_t argv[2];
    argv[0] = 8;
    /* Create an instance of the WASM module (WASM linear memory is ready) */
    /* Lookup a WASM function by its name */
    containers[container_id].ocre_runtime_arguments.func =
        wasm_runtime_lookup_function(containers[container_id].ocre_runtime_arguments.module_inst, "on_init");
    if (NULL == containers[container_id].ocre_runtime_arguments.func) {
        LOG_ERR("ERROR lookup function: ");
    }

    /* creat an execution environment to execute the WASM functions */
    containers[container_id].ocre_runtime_arguments.exec_env =
            wasm_runtime_create_exec_env(containers[container_id].ocre_runtime_arguments.module_inst,
                                         containers[container_id].ocre_runtime_arguments.stack_size);
    if (NULL == containers[container_id].ocre_runtime_arguments.exec_env) {
        LOG_ERR("ERROR creating executive environment: ");
    }

    /* call the WASM function */
    if (!wasm_runtime_call_wasm(containers[container_id].ocre_runtime_arguments.exec_env,
                                containers[container_id].ocre_runtime_arguments.func, 1, argv)) {
        LOG_ERR("ERROR calling main:");
    }

    containers[container_id].container_runtime_status = CONTAINER_STATUS_RUNNING;
    return CONTAINER_STATUS_RUNNING;
}

ocre_container_status_t ocre_container_runtime_destroy_container(ocre_cs_ctx *ctx, int container_id) {
    wasm_runtime_unload(containers[container_id].ocre_runtime_arguments.module);
    containers[container_id].container_runtime_status = CONTAINER_STATUS_DESTROYED;
    return CONTAINER_STATUS_DESTROYED;
}

ocre_container_status_t ocre_container_runtime_stop_container(ocre_cs_ctx *ctx, int container_id) {
    wasm_runtime_deinstantiate(containers[container_id].ocre_runtime_arguments.module_inst);
    containers[container_id].container_runtime_status = CONTAINER_STATUS_STOPPED;
    return CONTAINER_STATUS_STOPPED;
}

ocre_container_status_t ocre_container_runtime_restart_container(ocre_cs_ctx *ctx, int container_id) {
    ocre_container_runtime_stop_container(ctx, container_id);
    ocre_container_runtime_run_container(ctx, container_id);
    ocre_healthcheck_reinit(&containers[container_id].ocre_container_data.WDT);
    containers[container_id].container_runtime_status = CONTAINER_STATUS_RUNNING;
    return CONTAINER_STATUS_RUNNING;
}

ocre_container_status_t ocre_container_runtime_get_container_status(ocre_cs_ctx *ctx, int container_id) {
    return containers[container_id].container_runtime_status;
}

