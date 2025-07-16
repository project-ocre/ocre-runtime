/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ocre/ocre.h>
#include "ocre_core_external.h"

#ifdef CONFIG_OCRE_TIMER
#include "ocre_timers/ocre_timer.h"
#endif
#ifdef CONFIG_OCRE_GPIO
#include "ocre_gpio/ocre_gpio.h"
#endif
#ifdef CONFIG_OCRE_CONTAINER_MESSAGING
#include "container_messaging/messaging.h"
#endif
#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS)
#include "api/ocre_common.h"
#endif

#ifdef CONFIG_OCRE_SHELL
#include "ocre/shell/ocre_shell.h"
#endif

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#include "../../../../../wasm-micro-runtime/core/iwasm/include/lib_export.h"
#include "bh_log.h"
#include <stdlib.h>
#include "cs_sm.h"
#include "cs_sm_impl.h"

static char filepath[FILE_PATH_MAX];
static char wamr_heap_buf[CONFIG_OCRE_WAMR_HEAP_BUFFER_SIZE] = {0};

// Thread pool for container execution
#define CONTAINER_THREAD_POOL_SIZE 4
static core_thread_t container_threads[CONTAINER_THREAD_POOL_SIZE];
static bool container_thread_active[CONTAINER_THREAD_POOL_SIZE] = {false};

static core_mutex_t container_mutex;

// Arguments for container threads
struct container_thread_args {
    ocre_container_t *container;
};

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

static bool validate_container_memory(ocre_container_t *container) {
#ifdef CONFIG_OCRE_MEMORY_CHECK_ENABLED
    size_t requested_heap = container->ocre_container_data.heap_size;
    size_t requested_stack = container->ocre_container_data.stack_size;

    if (requested_heap == 0 || requested_heap > CONFIG_OCRE_CONTAINER_DEFAULT_HEAP_SIZE) {
        LOG_ERR("Invalid heap size requested: %zu bytes", requested_heap);
        return false;
    }

    if (requested_stack == 0 || requested_stack > CONFIG_OCRE_CONTAINER_DEFAULT_STACK_SIZE) {
        LOG_ERR("Invalid stack size requested: %zu bytes", requested_stack);
        return false;
    }

    size_t available_memory = ocre_get_available_memory();
    size_t required_memory = requested_heap + requested_stack + sizeof(WASMExecEnv);

    if (available_memory < required_memory) {
        LOG_ERR("Insufficient memory for container %d: required %zu bytes, available %zu bytes",
                container->container_ID, required_memory, available_memory);
        return false;
    }
    LOG_INF("Memory check passed: %zu bytes required, %zu bytes available", required_memory, available_memory);

    container->ocre_runtime_arguments.stack_size = requested_stack;
    container->ocre_runtime_arguments.heap_size = requested_heap;
#else
    container->ocre_runtime_arguments.stack_size = CONFIG_OCRE_CONTAINER_DEFAULT_STACK_SIZE;
    container->ocre_runtime_arguments.heap_size = CONFIG_OCRE_CONTAINER_DEFAULT_HEAP_SIZE;
#endif
    return true;
}

// Container thread entry function
static void container_thread_entry(struct container_thread_args *args) {
    ocre_container_t *container = args->container;
    wasm_module_inst_t module_inst = container->ocre_runtime_arguments.module_inst;

    // Initialize WASM runtime thread environment
    wasm_runtime_init_thread_env();

    LOG_INF("Container thread %d started", container->container_ID);

#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS)
    // Set TLS for the container's WASM module
    current_module_tls = &module_inst;
#endif
    // Run the WASM main function
    bool success = wasm_application_execute_main(module_inst, 0, NULL);

    // Update container status
    container->container_runtime_status = success ? CONTAINER_STATUS_STOPPED : CONTAINER_STATUS_ERROR;

    // Cleanup sequence
    core_mutex_lock(&container->lock);
    {
        // Cleanup timers and GPIO resources
#ifdef CONFIG_OCRE_TIMER
        ocre_timer_cleanup_container(module_inst);
#endif
#ifdef CONFIG_OCRE_GPIO
        ocre_gpio_cleanup_container(module_inst);
#endif

        // Clear thread tracking
        container_thread_active[container->container_ID] = false;
#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS)
        // Clear TLS
        current_module_tls = NULL;
#endif

    }
    core_mutex_unlock(&container->lock);

    LOG_INF("Container thread %d exited cleanly", container->container_ID);

    // Clean up WASM runtime thread environment
    wasm_runtime_destroy_thread_env();
    core_free(args); // Free the dynamically allocated args
}

static int get_available_thread(void) {
    for (int i = 0; i < CONTAINER_THREAD_POOL_SIZE; i++) {
        if (!container_thread_active[i]) {
            return i;
        }
    }
    return -1;
}

static int load_binary_to_buffer_fs(ocre_runtime_arguments_t *container_arguments,
                                    ocre_container_data_t *container_data) {
    int ret = 0;
    size_t file_size = 0;
    void *file_handle = NULL;
    char filepath[FILE_PATH_MAX];

    ret = core_construct_filepath(filepath, sizeof(filepath), container_data->sha256);
    if (ret < 0) {
        LOG_ERR("Failed to construct filepath for %s: %d", container_data->sha256, ret);
        return ret;
    }
    ret = core_filestat(filepath, &file_size);
    if (ret < 0) {
        LOG_ERR("Failed to get file status for %s: %d", filepath, ret);
        return ret;
    }

    container_arguments->size = file_size;
    container_arguments->buffer = malloc(file_size);
    if (!container_arguments->buffer) {
        LOG_ERR("Failed to allocate memory for container binary.");
        return -ENOMEM;
    }

    LOG_INF("File path: %s, size: %d", filepath, file_size);

    ret = core_fileopen(filepath, &file_handle);
    if (ret < 0) {
        LOG_ERR("Failed to open file %s: %d", filepath, ret);
        core_free(container_arguments->buffer);
        return ret;
    }

    ret = core_fileread(file_handle, container_arguments->buffer, file_size);
    if (ret < 0) {
        LOG_ERR("Failed to read file %s: %d", filepath, ret);
        core_fileclose(file_handle);
        core_free(container_arguments->buffer);
        return ret;
    }

    ret = core_fileclose(file_handle);
    if (ret < 0) {
        LOG_ERR("Failed to close file %s: %d", filepath, ret);
        core_free(container_arguments->buffer);
        return ret;
    }
    return 0;
}

int CS_ctx_init(ocre_cs_ctx *ctx) {
    for (int i = 0; i < CONFIG_MAX_CONTAINERS; i++) {
        core_mutex_init(&ctx->containers[i].lock);
        ctx->containers[i].container_runtime_status = CONTAINER_STATUS_UNKNOWN;
        ctx->containers[i].ocre_container_data.heap_size = CONFIG_OCRE_CONTAINER_DEFAULT_HEAP_SIZE;
        ctx->containers[i].ocre_container_data.stack_size = CONFIG_OCRE_CONTAINER_DEFAULT_STACK_SIZE;
        memset(ctx->containers[i].ocre_container_data.name, 0, sizeof(ctx->containers[i].ocre_container_data.name));
        memset(ctx->containers[i].ocre_container_data.sha256, 0, sizeof(ctx->containers[i].ocre_container_data.sha256));
        ctx->containers[i].ocre_container_data.timers = 0;
        ctx->containers[i].container_ID = i;
    }
#ifdef CONFIG_OCRE_SHELL
    register_ocre_shell(ctx);
#endif
    core_mutex_init(&container_mutex);
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

    bh_log_set_verbose_level(BH_LOG_LEVEL_WARNING);

    if (!wasm_runtime_register_natives("env", ocre_api_table, ocre_api_table_size)) {
        LOG_ERR("Failed to register the API's");
        return RUNTIME_STATUS_ERROR;
    }
#ifdef CONFIG_OCRE_TIMER
    ocre_timer_init();
#endif
#ifdef CONFIG_OCRE_CONTAINER_MESSAGING
    ocre_msg_system_init();
#endif
    return RUNTIME_STATUS_INITIALIZED;
}

ocre_container_runtime_status_t CS_runtime_destroy(void) {
    // Signal event threads to exit gracefully
#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS)
    ocre_common_shutdown();
#endif

    // Abort any active container threads
    for (int i = 0; i < CONTAINER_THREAD_POOL_SIZE; i++) {
        if (container_thread_active[i]) {
            core_thread_destroy(&container_threads[i]);
            container_thread_active[i] = false;
        }
    }
    return RUNTIME_STATUS_DESTROYED;
}

ocre_container_status_t CS_create_container(ocre_container_t *container) {
    uint32_t curr_container_ID = container->container_ID;
    ocre_container_data_t *curr_container_data = &container->ocre_container_data;
    ocre_runtime_arguments_t *curr_container_arguments = &container->ocre_runtime_arguments;

    if (container->container_runtime_status == CONTAINER_STATUS_STOPPED) {
        LOG_WRN("Container %d is in STOPPED state, destroying it before reuse", curr_container_ID);
        CS_destroy_container(container, NULL);
    }

    if (container->container_runtime_status != CONTAINER_STATUS_UNKNOWN &&
        container->container_runtime_status != CONTAINER_STATUS_DESTROYED) {
        LOG_ERR("Cannot create container again container with ID: %d, already exists", curr_container_ID);
        return RUNTIME_STATUS_ERROR;
    }

    if (!validate_container_memory(container)) {
        LOG_ERR("Memory check not passed");
        return CONTAINER_STATUS_ERROR;
    }

    LOG_INF("Allocating memory for container %d", curr_container_ID);
    int ret = load_binary_to_buffer_fs(curr_container_arguments, curr_container_data);
    if (ret < 0) {
        LOG_ERR("Failed to load binary to buffer: %d", ret);
        return CONTAINER_STATUS_ERROR;
    }
    LOG_INF("Loaded binary to buffer for container %d", curr_container_ID);

    curr_container_arguments->module =
            wasm_runtime_load(curr_container_arguments->buffer, curr_container_arguments->size,
                              curr_container_arguments->error_buf, sizeof(curr_container_arguments->error_buf));
    if (!curr_container_arguments->module) {
        LOG_ERR("Failed to load WASM module: %s", curr_container_arguments->error_buf);
        free(curr_container_arguments->buffer);
        return CONTAINER_STATUS_ERROR;
    }

    container->container_runtime_status = CONTAINER_STATUS_CREATED;
    LOG_WRN("Created container:%d", curr_container_ID);
    return container->container_runtime_status;
}

ocre_container_status_t CS_run_container(ocre_container_t *container) {
    uint32_t curr_container_ID = container->container_ID;
    ocre_runtime_arguments_t *curr_container_arguments = &container->ocre_runtime_arguments;

    if (container->container_runtime_status == CONTAINER_STATUS_RUNNING) {
        LOG_WRN("Container status is already in RUNNING state");
        return CONTAINER_STATUS_RUNNING;
    }

#ifdef CONFIG_OCRE_NETWORKING
            #define ADDRESS_POOL_SIZE 1
            const char *addr_pool[ADDRESS_POOL_SIZE] = {
                "0.0.0.0/0",
            };
            wasm_runtime_set_wasi_addr_pool(curr_container_arguments->module, addr_pool, ADDRESS_POOL_SIZE);
#endif


    if (container->container_runtime_status != CONTAINER_STATUS_CREATED &&
        container->container_runtime_status != CONTAINER_STATUS_STOPPED) {
        LOG_ERR("Container (ID: %d), is not in a valid state to run", curr_container_ID);
        container->container_runtime_status = CONTAINER_STATUS_ERROR;
        return CONTAINER_STATUS_ERROR;
    }

    if (curr_container_arguments->module_inst) {
        LOG_INF("WASM runtime already instantiated for container:%d", curr_container_ID);
    } else {
        LOG_INF("Instantiating WASM runtime for container:%d", curr_container_ID);
        curr_container_arguments->module_inst =
                wasm_runtime_instantiate(curr_container_arguments->module, curr_container_arguments->stack_size,
                                         curr_container_arguments->heap_size, curr_container_arguments->error_buf,
                                         sizeof(curr_container_arguments->error_buf));
        if (!curr_container_arguments->module_inst) {
            LOG_ERR("Failed to instantiate WASM module: %s, for containerID= %d", curr_container_arguments->error_buf,
                    curr_container_ID);
            wasm_runtime_unload(curr_container_arguments->module);
            free(curr_container_arguments->buffer);
            return CONTAINER_STATUS_ERROR;
        }
#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS)
        ocre_register_module(curr_container_arguments->module_inst);
#endif
    }

    core_mutex_lock(&container_mutex);
    int thread_idx = get_available_thread();
    if (thread_idx == -1) {
        LOG_ERR("No available threads for container %d", curr_container_ID);
        container->container_runtime_status = CONTAINER_STATUS_ERROR;
        core_mutex_unlock(&container_mutex);
        return CONTAINER_STATUS_ERROR;
    }

    // Allocate thread arguments dynamically
    struct container_thread_args *args = core_malloc(sizeof(struct container_thread_args));
    if (!args) {
        LOG_ERR("Failed to allocate thread args for container %d", curr_container_ID);
        container->container_runtime_status = CONTAINER_STATUS_ERROR;
        core_mutex_unlock(&container_mutex);
        return CONTAINER_STATUS_ERROR;
    }
    args->container = container;

    // Create and start a new thread for the container
    char thread_name[16];
    snprintf(thread_name, sizeof(thread_name), "container_%d", curr_container_ID);
    container_threads[thread_idx].user_options = curr_container_ID;
    int ret = core_thread_create(&container_threads[thread_idx], container_thread_entry, args, thread_name,
                                 CONTAINER_THREAD_STACK_SIZE, 5);

    if (ret != 0) {
        LOG_ERR("Failed to create thread for container %d", curr_container_ID);
        container->container_runtime_status = CONTAINER_STATUS_ERROR;

        core_free(args); // Free the dynamically allocated args

        core_mutex_unlock(&container_mutex);
        return CONTAINER_STATUS_ERROR;
    }

    container_thread_active[thread_idx] = true;
    core_mutex_unlock(&container_mutex);

    container->container_runtime_status = CONTAINER_STATUS_RUNNING;
    LOG_WRN("Running container:%d in dedicated thread", curr_container_ID);
    return CONTAINER_STATUS_RUNNING;
}

ocre_container_status_t CS_get_container_status(ocre_cs_ctx *ctx, int container_id) {
    if (container_id < 0 || container_id >= CONFIG_MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    return ctx->containers[container_id].container_runtime_status;
}

ocre_container_status_t CS_stop_container(ocre_container_t *container, ocre_container_runtime_cb callback) {
    uint32_t curr_container_ID = container->container_ID;
    ocre_runtime_arguments_t *curr_container_arguments = &container->ocre_runtime_arguments;
    if (container->container_runtime_status == CONTAINER_STATUS_STOPPED) {
        LOG_WRN("Container status is already in STOP state");
        return CONTAINER_STATUS_STOPPED;
    }
    core_mutex_lock(&container->lock);
    {
        LOG_INF("Stopping container %d from state %d", curr_container_ID, container->container_runtime_status);

        for (int i = 0; i < CONTAINER_THREAD_POOL_SIZE; i++) {
            if (container_thread_active[i] && container_threads[i].user_options == curr_container_ID) {
                core_thread_destroy(&container_threads[i]);
                container_thread_active[i] = false;
            }
        }
#ifdef CONFIG_OCRE_TIMER
        ocre_timer_cleanup_container(curr_container_arguments->module_inst);
#endif
#ifdef CONFIG_OCRE_GPIO
        ocre_gpio_cleanup_container(curr_container_arguments->module_inst);
#endif
#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS)
        ocre_unregister_module(curr_container_arguments->module_inst);
#endif
        if (curr_container_arguments->module_inst) {
            wasm_runtime_deinstantiate(curr_container_arguments->module_inst);
            curr_container_arguments->module_inst = NULL;
        }

        container->container_runtime_status = CONTAINER_STATUS_STOPPED;
    }
    core_mutex_unlock(&container->lock);

    if (callback) {
        callback();
    }
    return CONTAINER_STATUS_STOPPED;
}

ocre_container_status_t CS_destroy_container(ocre_container_t *container, ocre_container_runtime_cb callback) {
    core_mutex_lock(&container->lock);
    {
        LOG_INF("Destroying container %d", container->container_ID);

        if (container->container_runtime_status != CONTAINER_STATUS_STOPPED) {
            CS_stop_container(container, NULL);
        }

        if (container->ocre_runtime_arguments.module) {
            wasm_runtime_unload(container->ocre_runtime_arguments.module);
            container->ocre_runtime_arguments.module = NULL;
        }

        if (container->ocre_runtime_arguments.buffer) {
            free(container->ocre_runtime_arguments.buffer);
            container->ocre_runtime_arguments.buffer = NULL;
        }

        memset(&container->ocre_container_data, 0, sizeof(ocre_container_data_t));
        container->container_runtime_status = CONTAINER_STATUS_DESTROYED;
    }
    core_mutex_unlock(&container->lock);

    if (callback) {
        callback();
    }
    return CONTAINER_STATUS_DESTROYED;
}

ocre_container_status_t CS_restart_container(ocre_container_t *container, ocre_container_runtime_cb callback) {
    ocre_container_status_t status = CS_stop_container(container, NULL);
    if (status != CONTAINER_STATUS_STOPPED) {
        LOG_ERR("Failed to stop container: %d", container->container_ID);
        return CONTAINER_STATUS_ERROR;
    }

    status = CS_run_container(container);
    if (status != CONTAINER_STATUS_RUNNING) {
        LOG_ERR("Failed to start container: %d", container->container_ID);
        return CONTAINER_STATUS_ERROR;
    }
    if (callback) {
        callback();
    }

    return CONTAINER_STATUS_RUNNING;
}
