/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel/mm.h>
#include <stdlib.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);
#include <autoconf.h>
#include "../../../../../wasm-micro-runtime/core/iwasm/include/lib_export.h"
#include "bh_log.h"

#include "cs_sm.h"
#include "cs_sm_impl.h"
#include "../../ocre_timers/ocre_timer.h"
#include "../../container_messaging/messaging.h"

static char filepath[FILE_PATH_MAX];
static char wamr_heap_buf[CONFIG_OCRE_WAMR_HEAP_BUFFER_SIZE] = {0};

// Thread stacks for containers
#define CONTAINER_THREAD_STACK_SIZE 4096
static K_THREAD_STACK_ARRAY_DEFINE(container_thread_stacks, MAX_CONTAINERS, CONTAINER_THREAD_STACK_SIZE);
static struct k_thread container_threads[MAX_CONTAINERS];
static k_tid_t container_thread_ids[MAX_CONTAINERS];
static bool container_thread_active[MAX_CONTAINERS] = {false};

#ifdef CONFIG_OCRE_MEMORY_CHECK_ENABLED
static size_t ocre_get_available_memory(void) {
#ifdef CONFIG_HEAP_MEM_POOL_SIZE
    struct k_heap_stats stats;
    k_heap_sys_get_stats(&stats);
    return stats.free_bytes;
#else
    extern char *z_malloc_mem_pool_area_start;
    extern char *z_malloc_mem_pool_area_end;
    extern struct sys_mem_pool_base z_malloc_mem_pool;

    size_t total_size = z_malloc_mem_pool_area_end - z_malloc_mem_pool_area_start;
    size_t used_size = sys_mem_pool_get_used_size(&z_malloc_mem_pool);
    return total_size - used_size;
#endif
}
#endif

// Thread entry point for container execution
static void container_thread_entry(void *container_id_ptr, void *ctx_ptr, void *unused) {
    int container_id = *((int *)container_id_ptr);
    ocre_cs_ctx *ctx = (ocre_cs_ctx *)ctx_ptr;

    LOG_INF("Container thread %d started", container_id);
    if (wasm_application_execute_main(ctx->containers[container_id].ocre_runtime_arguments.module_inst, 0, NULL)) {
        LOG_INF("Container %d main function completed successfully", container_id);
    } else {
        LOG_ERR("ERROR calling main for container %d: %s", container_id,
                wasm_runtime_get_exception(ctx->containers[container_id].ocre_runtime_arguments.module_inst));
        ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_ERROR;
    }

    // Clean up timers when container exits (whether normally or due to error)
    ocre_timer_cleanup_container(ctx->containers[container_id].ocre_runtime_arguments.module_inst);

    LOG_INF("Container thread %d exiting", container_id);
    container_thread_active[container_id] = false;

    if (ctx->containers[container_id].container_runtime_status == CONTAINER_STATUS_RUNNING) {
        ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_STOPPED;
        LOG_INF("Container %d marked as STOPPED after main exit", container_id);
    }
}

static int load_binary_to_buffer_fs(ocre_cs_ctx *ctx, int container_id, ocre_container_data_t *container_data) {
    int ret = 0;
    struct fs_file_t app_file;
    struct fs_dirent entry;

    snprintf(filepath, sizeof(filepath), "/lfs/ocre/images/%s.bin", container_data->sha256);

    ret = fs_stat(filepath, &entry);
    if (ret < 0) {
        LOG_ERR("Failed to get file status for %s: %d", filepath, ret);
        return ret;
    }

    ctx->containers[container_id].ocre_runtime_arguments.size = entry.size;
    LOG_INF("Allocating memory for container %d", container_id);
    ctx->containers[container_id].ocre_runtime_arguments.buffer = (char *)malloc(entry.size * sizeof(char));
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
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        ctx->containers[i].container_runtime_status = CONTAINER_STATUS_UNKNOWN;
        ctx->containers[i].ocre_container_data.heap_size = CONFIG_OCRE_CONTAINER_DEFAULT_HEAP_SIZE;
        ctx->containers[i].ocre_container_data.stack_size = CONFIG_OCRE_CONTAINER_DEFAULT_STACK_SIZE;
        memset(ctx->containers[i].ocre_container_data.name, 0, sizeof(ctx->containers[i].ocre_container_data.name));
        memset(ctx->containers[i].ocre_container_data.sha256, 0, sizeof(ctx->containers[i].ocre_container_data.sha256));
        ctx->containers[i].ocre_container_data.timers = 0;
        ctx->containers[i].ocre_container_data.watchdog_interval = 0;
        container_thread_active[i] = false;
        container_thread_ids[i] = NULL;
    }

    return 0;
}

ocre_container_runtime_status_t CS_runtime_init(ocre_cs_ctx *ctx, ocre_container_init_arguments_t *args) {
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = wamr_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(wamr_heap_buf);
    init_args.native_module_name = "env";
    init_args.n_native_symbols = ocre_api_table_size;
    init_args.native_symbols = ocre_api_table;

    if (!wasm_runtime_full_init(&init_args)) {
        LOG_ERR("Failed to initialize the WASM runtime");
        return RUNTIME_STATUS_ERROR;
    }

    // Configure WAMR logging
    bh_log_set_verbose_level(BH_LOG_LEVEL_WARNING);

    if (!wasm_runtime_register_natives("env", ocre_api_table, ocre_api_table_size)) {
        LOG_ERR("Failed to register the API's");
        return RUNTIME_STATUS_ERROR;
    }
    ocre_timer_init();
    ocre_msg_system_init();
    return RUNTIME_STATUS_INITIALIZED;
}

ocre_container_runtime_status_t CS_runtime_destroy(void) {
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (container_thread_active[i] && container_thread_ids[i] != NULL) {
            k_thread_abort(container_thread_ids[i]);
            container_thread_active[i] = false;
            container_thread_ids[i] = NULL;
        }
    }
    return RUNTIME_STATUS_DESTROYED;
}

ocre_container_status_t CS_create_container(ocre_cs_ctx *ctx, int container_id) {
    if (ctx->containers[container_id].container_runtime_status == CONTAINER_STATUS_STOPPED) {
        LOG_WRN("Container %d is in STOPPED state, destroying it before reuse", container_id);
        CS_destroy_container(ctx, container_id, NULL);
    }

    if (ctx->containers[container_id].container_runtime_status == CONTAINER_STATUS_DESTROYED ||
        ctx->containers[container_id].container_runtime_status == CONTAINER_STATUS_UNKNOWN) {

#ifdef CONFIG_OCRE_MEMORY_CHECK_ENABLED
        size_t requested_heap = ctx->containers[container_id].ocre_container_data.heap_size;
        size_t requested_stack = ctx->containers[container_id].ocre_container_data.stack_size;

        if (requested_heap == 0 || requested_heap > CONFIG_OCRE_CONTAINER_DEFAULT_HEAP_SIZE) {
            LOG_ERR("Invalid heap size requested: %zu bytes", requested_heap);
            return CONTAINER_STATUS_ERROR;
        }

        if (requested_stack == 0 || requested_stack > CONFIG_OCRE_CONTAINER_DEFAULT_STACK_SIZE) {
            LOG_ERR("Invalid stack size requested: %zu bytes", requested_stack);
            return CONTAINER_STATUS_ERROR;
        }

        size_t available_memory = ocre_get_available_memory();
        size_t required_memory = requested_heap + requested_stack + sizeof(WASMExecEnv);

        if (available_memory < required_memory) {
            LOG_ERR("Insufficient memory for container %d: required %zu bytes, available %zu bytes", container_id,
                    required_memory, available_memory);
            return CONTAINER_STATUS_ERROR;
        }
        LOG_INF("Memory check passed: %zu bytes required, %zu bytes available", required_memory, available_memory);
#endif

        ctx->containers[container_id].ocre_runtime_arguments.stack_size =
                ctx->containers[container_id].ocre_container_data.stack_size;
        ctx->containers[container_id].ocre_runtime_arguments.heap_size =
                ctx->containers[container_id].ocre_container_data.heap_size;
#ifndef CONFIG_OCRE_MEMORY_CHECK_ENABLED
        ctx->containers[container_id].ocre_runtime_arguments.stack_size = CONFIG_OCRE_CONTAINER_DEFAULT_STACK_SIZE;
        ctx->containers[container_id].ocre_runtime_arguments.heap_size = CONFIG_OCRE_CONTAINER_DEFAULT_HEAP_SIZE;
#endif

        int ret = load_binary_to_buffer_fs(ctx, container_id, &ctx->containers[container_id].ocre_container_data);
        if (ret < 0) {
            LOG_ERR("Failed to load binary to buffer: %d", ret);
            return CONTAINER_STATUS_ERROR;
        }
        LOG_WRN("Loaded binary to buffer for container %d", container_id);
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
        if (ctx->containers[container_id].ocre_runtime_arguments.module_inst) {
            LOG_INF("WASM runtime allready instantiated for container:%d", container_id);
        } else {
            LOG_INF("instantiating WASM runtime for container:%d", container_id);
            ctx->containers[container_id].ocre_runtime_arguments.module_inst =
                    wasm_runtime_instantiate(ctx->containers[container_id].ocre_runtime_arguments.module,
                                             ctx->containers[container_id].ocre_runtime_arguments.stack_size,
                                             ctx->containers[container_id].ocre_runtime_arguments.heap_size,
                                             ctx->containers[container_id].ocre_runtime_arguments.error_buf,
                                             sizeof(ctx->containers[container_id].ocre_runtime_arguments.error_buf));
            if (!ctx->containers[container_id].ocre_runtime_arguments.module_inst) {
                LOG_ERR("Failed to instantiate WASM module: %s, for containerID= %d",
                        ctx->containers[container_id].ocre_runtime_arguments.error_buf, container_id);
                return CONTAINER_STATUS_ERROR;
            }
        }

        ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_CREATED;
        LOG_WRN("Created container:%d", container_id);
        return ctx->containers[container_id].container_runtime_status;
    } else {
        LOG_ERR("Cannot create container again container with ID: %d, already exists", container_id);
        return RUNTIME_STATUS_ERROR;
    }
}

ocre_container_status_t CS_run_container(ocre_cs_ctx *ctx, int *container_id) {
    char current_sha256[70 + 1];

    if (ctx->containers[*container_id].container_runtime_status == CONTAINER_STATUS_STOPPED) {
        strcpy(current_sha256, ctx->containers[*container_id].ocre_container_data.sha256);

        if (strcmp(current_sha256, ctx->containers[*container_id].ocre_container_data.sha256) != 0) {
            LOG_WRN("Running different app in container %d, performing full cleanup", *container_id);
            CS_destroy_container(ctx, *container_id, NULL);
            CS_create_container(ctx, *container_id);
        }
    }

    if (ctx->containers[*container_id].container_runtime_status == CONTAINER_STATUS_CREATED ||
        ctx->containers[*container_id].container_runtime_status == CONTAINER_STATUS_STOPPED) {
        ocre_timer_set_module_inst(ctx->containers[*container_id].ocre_runtime_arguments.module_inst);
        ocre_gpio_set_module_inst(ctx->containers[*container_id].ocre_runtime_arguments.module_inst);

        ctx->containers[*container_id].ocre_runtime_arguments.exec_env =
                wasm_runtime_create_exec_env(ctx->containers[*container_id].ocre_runtime_arguments.module_inst,
                                             ctx->containers[*container_id].ocre_runtime_arguments.stack_size);
        if (NULL == ctx->containers[*container_id].ocre_runtime_arguments.exec_env) {
            LOG_ERR("ERROR creating executive environment for container %d", *container_id);
            ctx->containers[*container_id].container_runtime_status = CONTAINER_STATUS_ERROR;
            return CONTAINER_STATUS_ERROR;
        }
        if (container_thread_active[*container_id]) {
            LOG_ERR("Container %d thread is already active", *container_id);
            return CONTAINER_STATUS_ERROR;
        }
        container_thread_ids[*container_id] = k_thread_create(
                &container_threads[*container_id], container_thread_stacks[*container_id], CONTAINER_THREAD_STACK_SIZE,
                container_thread_entry, container_id, ctx, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
        if (container_thread_ids[*container_id] == NULL) {
            LOG_ERR("Failed to create thread for container %d", *container_id);
            ctx->containers[*container_id].container_runtime_status = CONTAINER_STATUS_ERROR;
            return CONTAINER_STATUS_ERROR;
        }
        container_thread_active[*container_id] = true;
        ctx->containers[*container_id].container_runtime_status = CONTAINER_STATUS_RUNNING;
        char thread_name[16];
        snprintf(thread_name, sizeof(thread_name), "container_%d", *container_id);
        k_thread_name_set(container_thread_ids[*container_id], thread_name);
        LOG_WRN("Running container:%d in dedicated thread", *container_id);
        return CONTAINER_STATUS_RUNNING;
    } else {
        LOG_ERR("Container (ID: %d), does not exist to run it", *container_id);
        ctx->containers[*container_id].container_runtime_status = CONTAINER_STATUS_ERROR;
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

    if (ctx->containers[container_id].container_runtime_status == CONTAINER_STATUS_RUNNING) {
        if (container_thread_active[container_id] && container_thread_ids[container_id] != NULL) {
            LOG_INF("Aborting thread for container %d", container_id);
            k_thread_abort(container_thread_ids[container_id]);
            container_thread_active[container_id] = false;
            container_thread_ids[container_id] = NULL;
        }

        if (ctx->containers[container_id].ocre_runtime_arguments.exec_env) {
            wasm_runtime_destroy_exec_env(ctx->containers[container_id].ocre_runtime_arguments.exec_env);
            ctx->containers[container_id].ocre_runtime_arguments.exec_env = NULL;
        }

        wasm_runtime_deinstantiate(ctx->containers[container_id].ocre_runtime_arguments.module_inst);
        ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_STOPPED;
        LOG_WRN("Stopped container:%d", container_id);

        if (callback) {
            callback();
        }

        return CONTAINER_STATUS_STOPPED;
    } else {
        LOG_ERR("Container %d, is not running to stop it", container_id);
        ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_ERROR;
        return CONTAINER_STATUS_ERROR;
    }
}

ocre_container_status_t CS_destroy_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback) {
    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    ocre_timer_cleanup_container(ctx->containers[container_id].ocre_runtime_arguments.module_inst);

    if (ctx->containers[container_id].container_runtime_status == CONTAINER_STATUS_UNKNOWN) {
        LOG_ERR("Cannot destroy container %d: It was never created.", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    if (ctx->containers[container_id].container_runtime_status != CONTAINER_STATUS_STOPPED) {
        LOG_WRN("Container %d is not stopped yet. Stopping it first.", container_id);
        CS_stop_container(ctx, container_id, NULL);
    }

    if (container_thread_active[container_id] && container_thread_ids[container_id] != NULL) {
        k_thread_abort(container_thread_ids[container_id]);
        container_thread_active[container_id] = false;
        container_thread_ids[container_id] = NULL;
    }

    if (ctx->containers[container_id].ocre_runtime_arguments.exec_env) {
        wasm_runtime_destroy_exec_env(ctx->containers[container_id].ocre_runtime_arguments.exec_env);
        ctx->containers[container_id].ocre_runtime_arguments.exec_env = NULL;
        LOG_INF("Destroyed exec environment for container %d", container_id);
    }

    if (ctx->containers[container_id].ocre_runtime_arguments.module_inst) {
        wasm_runtime_deinstantiate(ctx->containers[container_id].ocre_runtime_arguments.module_inst);
        ctx->containers[container_id].ocre_runtime_arguments.module_inst = NULL;
        LOG_INF("Deinstantiated WASM module for container %d", container_id);
    }

    if (ctx->containers[container_id].ocre_runtime_arguments.module) {
        wasm_runtime_unload(ctx->containers[container_id].ocre_runtime_arguments.module);
        ctx->containers[container_id].ocre_runtime_arguments.module = NULL;
    }
    if (ctx->containers[container_id].ocre_runtime_arguments.buffer) {
        free(ctx->containers[container_id].ocre_runtime_arguments.buffer);
        LOG_INF("Freed buffer for container %d", container_id);
        ctx->containers[container_id].ocre_runtime_arguments.buffer = NULL;
    }
    memset(ctx->containers[container_id].ocre_container_data.name, 0,
           sizeof(ctx->containers[container_id].ocre_container_data.name));
    memset(ctx->containers[container_id].ocre_container_data.sha256, 0,
           sizeof(ctx->containers[container_id].ocre_container_data.sha256));
    ctx->containers[container_id].ocre_container_data.timers = 0;
    ctx->containers[container_id].ocre_container_data.watchdog_interval = 0;

    wasm_runtime_unload(ctx->containers[container_id].ocre_runtime_arguments.module);
    free(ctx->containers[container_id].ocre_runtime_arguments.buffer);
    ctx->containers[container_id].ocre_runtime_arguments.buffer = NULL;

    LOG_WRN("Destroyed container: %d", container_id);
    ctx->containers[container_id].container_runtime_status = CONTAINER_STATUS_DESTROYED;

    if (callback) {
        callback();
    }

    return CONTAINER_STATUS_DESTROYED;
}

ocre_container_status_t CS_restart_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback) {
    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    ocre_container_status_t status = CS_stop_container(ctx, container_id, NULL);
    if (status != CONTAINER_STATUS_STOPPED) {
        LOG_ERR("Failed to stop container: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    // Start container with a new thread
    status = CS_run_container(ctx, container_id);
    if (status != CONTAINER_STATUS_RUNNING) {
        LOG_ERR("Failed to start container: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    if (ctx->containers[container_id].ocre_container_data.watchdog_interval > 0) {
        ocre_healthcheck_reinit(&ctx->containers[container_id].ocre_container_data.WDT);
    }

    if (callback) {
        callback();
    }

    return CONTAINER_STATUS_RUNNING;
}