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

// WAMR includes
#include "coap_ext.h"

#include "../api/ocre_api.h"

#include <ocre/ocre.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#include <ocre/fs/fs.h>
#include <ocre/sm/sm.h>
#include "ocre_container_runtime.h"

#include "../../components/container_supervisor/cs_sm.h"

#include "../container_healthcheck/ocre_container_healthcheck.h"

#define FS_MAX_PATH_LEN 256
struct k_thread ocre_cs_thread;

extern ocre_runime_container_ctx containers;
extern int curent_container_index;

static char filepath[FILE_PATH_MAX];
static char wamr_heap_buf[CONFIG_OCRE_WAMR_HEAP_BUFFER_SIZE] = {0};


static int load_binary_to_buffer_fs(char *file_path, ocre_container *container) {
    int ret = 0;
    struct fs_file_t app_file;

    struct fs_dirent entry;
    snprintf(filepath, sizeof(filepath), "/lfs/ocre/images/%s", file_path);
    ret = fs_stat(filepath, &entry);

    if (0 > ret) {
        LOG_ERR("Fail with file( %s ): %d", filepath, ret);
    }

    container->ocre_runtime_arguments.size = entry.size;

    container->ocre_runtime_arguments.buffer = malloc(entry.size * sizeof(char));
    LOG_INF("file path( %s ), size (%d): ", filepath, entry.size);
    fs_file_t_init(&app_file);

    ret = fs_open(&app_file, filepath, FS_O_READ);

    if (0 > ret) {
        LOG_ERR("Fail open file( %s ): %d", filepath, ret);
        return ret;
    }

    ret = fs_read(&app_file, container->ocre_runtime_arguments.buffer, container->ocre_runtime_arguments.size);

    if (0 > ret) {
        return ret;
    }

    ret = fs_close(&app_file);
    if (0 > ret) {
        return ret;
    }

    return ret;
}

void ocre_container_runtime_init() {
    struct ocre_message event;
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
    LOG_INF("Initialized OCRE_CR");
    event.event = EVENT_CREATE_CONTAINERS;
    ocre_component_send(&ocre_cs_component, &event);
}

ocre_container_status_t ocre_container_runtime_create_container(char *file_path, ocre_container *container) {
    struct ocre_message event;
    int ret = load_binary_to_buffer_fs(file_path, container);
    if (0 > ret) {
        LOG_ERR("Failed to load binary to buffer");
    }
    LOG_INF("Loaded binary to buffer (%d)", ret);
    // take the info about the app heap, timers, watchdogs needed for run
    container->ocre_runtime_arguments.stack_size = 4096;
    container->ocre_runtime_arguments.heap_size = 4096;
    /* parse the WASM file from buffer and create a WASM module */
    container->ocre_runtime_arguments.module = wasm_runtime_load(
            container->ocre_runtime_arguments.buffer, container->ocre_runtime_arguments.size,
            container->ocre_runtime_arguments.error_buf, sizeof(container->ocre_runtime_arguments.error_buf));
    if (!container->ocre_runtime_arguments.module) {
        LOG_ERR("ERROR load: %s", container->ocre_runtime_arguments.error_buf);
    }
    /* create an instance of the WASM module (WASM linear memory is ready) */
    container->ocre_runtime_arguments.module_inst = wasm_runtime_instantiate(
            container->ocre_runtime_arguments.module, container->ocre_runtime_arguments.stack_size,
            container->ocre_runtime_arguments.heap_size, container->ocre_runtime_arguments.error_buf,
            sizeof(container->ocre_runtime_arguments.error_buf));
    if (!container->ocre_runtime_arguments.module_inst) {
        LOG_ERR("ERROR instantiate: %s", container->ocre_runtime_arguments.error_buf);
    }
    event.event = EVENT_RUN_CONTAINERS;
    ocre_component_send(&ocre_cs_component, &event);
    container->container_runtime_status = CONTAINER_STATUS_CREATED;
    LOG_INF("Created Container: /lfs/ocre/%s", file_path);
    return CONTAINER_STATUS_CREATED;
}

ocre_container_status_t ocre_container_runtime_run_container(ocre_container *container) {
    /* create an instance of the WASM module (WASM linear memory is ready) */
    /* lookup a WASM function by its name the function signature can NULL here */
    container->ocre_runtime_arguments.func =
            wasm_runtime_lookup_function(container->ocre_runtime_arguments.module_inst, "on_init", "");
    if (NULL == container->ocre_runtime_arguments.func) {
        LOG_ERR("ERROR lookup function: ");
    }
    /* create an execution environment to execute the WASM functions */
    container->ocre_runtime_arguments.exec_env = wasm_runtime_create_exec_env(
            container->ocre_runtime_arguments.module_inst, container->ocre_runtime_arguments.stack_size);
    if (NULL == container->ocre_runtime_arguments.exec_env) {
        LOG_ERR("ERROR creating executive environment: ");
    }
    /* call the WASM function */
    uint32_t argv[2];
    argv[0] = 8;

    if (!wasm_runtime_call_wasm(container->ocre_runtime_arguments.exec_env, container->ocre_runtime_arguments.func, 1,
                                argv)) {
        LOG_ERR("ERROR calling main:");
    }
    container->container_runtime_status = CONTAINER_STATUS_RUNNING;

    return CONTAINER_STATUS_RUNNING;
}

ocre_container_status_t ocre_container_runtime_destroy_container(ocre_container *container) {
    wasm_runtime_unload(container->ocre_runtime_arguments.module);
    container->container_runtime_status = CONTAINER_STATUS_DESTROYED;
    return CONTAINER_STATUS_DESTROYED;
}

ocre_container_status_t ocre_container_runtime_get_container_status(ocre_container *container) {
    return container->container_runtime_status;
}

ocre_container_status_t ocre_container_runtime_stop_container(ocre_container *container) {
    wasm_runtime_deinstantiate(container->ocre_runtime_arguments.module_inst);
    container->container_runtime_status = CONTAINER_STATUS_STOPPED;
    return CONTAINER_STATUS_STOPPED;
}

ocre_container_status_t ocre_container_runtime_restart_container(ocre_container *container) {
    ocre_container_runtime_stop_container(container);
    ocre_container_runtime_run_container(container);
    ocre_healthcheck_reinit(&container->ocre_runtime_arguments.WDT);
    container->container_runtime_status = CONTAINER_STATUS_RUNNING;
    return CONTAINER_STATUS_RUNNING;
}

void ocre_container_get_file_count(int file_conut) {
}

void ocre_request_create_container() {
    LOG_INF("REQUEST_CREATE_CONTAINER_CATCH");
    struct ocre_message event;
    event.event = EVENT_CREATE_CONTAINERS;
    ocre_component_send(&ocre_cs_component, &event);
}

void ocre_container_unresponsive() {
}
