/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_CONTAINER_RUNTIME_H
#define OCRE_CONTAINER_RUNTIME_H

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#include <ocre/fs/fs.h>
#include <ocre/sm/sm.h>

#include "wasm_export.h"

#include "ocre_container_runtime.h"
#include "../container_healthcheck/ocre_container_healthcheck.h"
#include "../../components/container_supervisor/cs_sm.h"

#define MAX_CONTAINERS 10

#define OCRE_CR_DEBUG_ON 0
#define FILE_PATH_MAX    256

typedef struct ocre_runtime_arguments_t {
    uint32_t size;
    char *buffer;
    char error_buf[128];
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    wasm_function_inst_t func;
    wasm_exec_env_t exec_env;
    uint32_t stack_size;
    uint32_t heap_size;
    ocre_healthcheck WDT;
} ocre_runtime_arguments_t;

typedef enum {
    OCRE_CONTAINER_PERM_READ_ONLY,
    OCRE_CONTAINER_PERM_READ_WRITE,
    OCRE_CONTAINER_PERM_EXECUTE
} ocre_container_permissions_t;

typedef enum {
    CONTAINER_STATUS_UNKNOWN,
    CONTAINER_STATUS_PENDING,
    CONTAINER_STATUS_CREATED,
    CONTAINER_STATUS_RUNNING,
    CONTAINER_STATUS_STOPPED,
    CONTAINER_STATUS_DESTROYED,
    CONTAINER_STATUS_UNRESPONSIVE,
    CONTAINER_STATUS_ERROR,
} ocre_container_status_t;

typedef struct ocre_container {
    ocre_runtime_arguments_t ocre_runtime_arguments;
    ocre_container_status_t container_runtime_status;
} ocre_container;
typedef struct ocre_binary_t {
    char name[64];
} ocre_binary_t;
typedef struct ocre_runime_container_ctx {
    int file_count;
    ocre_binary_t ocre_binary;
    ocre_container container;
} ocre_runime_container_ctx;

/**
 * @brief Initializes the runtime itself.
 *
 * @return Void
 */
void ocre_container_runtime_init();

/**
 * @brief Create and initialize a container.
 *
 * @param container_id Identifier for the container.
 * @param image_name Name of the container image to be loaded.
 * @param container_config Additional configuration for the container.
 * @return Returns current status of container
 */
ocre_container_status_t ocre_container_runtime_create_container(char *file_path, ocre_container *container);

/**
 * @brief Runs a loaded container.
 *
 * @param container_id Identifier for the container to be run.
 * @return Returns 0 on success, negative error code on failure.
 */
ocre_container_status_t ocre_container_runtime_run_container(ocre_container *container);

/**
 * @brief Retrieves the status of a specific container.
 *
 * @param container_id Identifier for the container.
 * @return Returns the status of the specified container.
 */
ocre_container_status_t ocre_container_runtime_get_container_status(ocre_container *container);

/**
 * @brief Stops a running container.
 *
 * @param container_id Identifier for the container to be stopped.
 * @return Returns 0 on success, negative error code on failure.
 */
ocre_container_status_t ocre_container_runtime_stop_container(ocre_container *container);

/**
 * @brief Unloads a container.
 *
 * @param ocre_cs_ctx context of ocre runtime.
 * @return Returns 0 on success, negative error code on failure.
 */
ocre_container_status_t ocre_container_runtime_destroy_container(ocre_container *container);

/**
 * @brief Stops the container runtime.
 *
 * @return Returns 0 on success, negative error code on failure.
 */
ocre_container_status_t ocre_container_runtime_stop(ocre_container *container);

/**
 * @brief Restarts a running container.
 *
 * @param container_id Identifier for the container to be restarted.
 * @return Returns 0 on success, negative error code on failure.
 */
ocre_container_status_t ocre_container_runtime_restart_container(ocre_container *container);

void ocre_request_create_container();
void ocre_container_unresponsive();
void ocre_container_get_file_count(int file_conut);
#endif /* OCRE_CONTAINER_RUNTIME_H */