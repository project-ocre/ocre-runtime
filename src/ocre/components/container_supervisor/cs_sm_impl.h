/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_CS_SM_IMPL_H
#define OCRE_CS_SM_IMPL_H

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <ocre/fs/fs.h>
#include <ocre/sm/sm.h>
#include "../../api/ocre_api.h"
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>

// -----------------------------------------------------------------------------
// Function prototypes for OCRE container runtime and container management
// -----------------------------------------------------------------------------

// RUNTIME

/**
 * @brief Initialize the container runtime environment.
 *
 * This function sets up the runtime environment for OCRE containers.
 *
 * @param ctx  Pointer to the container runtime context.
 * @param args Pointer to initialization arguments.
 * @return ocre_container_runtime_status_t Status of the initialization.
 */
ocre_container_runtime_status_t CS_runtime_init(ocre_cs_ctx *ctx, ocre_container_init_arguments_t *args);

/**
 * @brief Destroy the container runtime environment.
 *
 * This function cleans up and releases any resources used by the container runtime.
 *
 * @return ocre_container_runtime_status_t Status of the destruction process.
 */
ocre_container_runtime_status_t CS_runtime_destroy(void);

// CONTAINER

/**
 * @brief Create a new container.
 *
 * This function creates a container with the specified container ID and associates it with the context.
 *
 * @param ctx          Pointer to the container runtime context.
 * @param container_id The ID of the container to be created.
 * @return ocre_container_status_t Status of the container creation.
 */
ocre_container_status_t CS_create_container(ocre_cs_ctx *ctx, int container_id);

/**
 * @brief Run a container.
 *
 * This function starts the execution of a container based on its container ID.
 *
 * @param ctx          Pointer to the container runtime context.
 * @param container_id The ID of the container to be run.
 * @return ocre_container_status_t Status of the container execution.
 */
ocre_container_status_t CS_run_container(ocre_cs_ctx *ctx, int *container_id);

/**
 * @brief Get the status of a container.
 *
 * This function retrieves the current status of the container.
 *
 * @param ctx          Pointer to the container runtime context.
 * @param container_id The ID of the container whose status is being queried.
 * @return ocre_container_status_t Status of the container.
 */
ocre_container_status_t CS_get_container_status(ocre_cs_ctx *ctx, int container_id);

/**
 * @brief Stop a container.
 *
 * This function stops the execution of the container and invokes a callback function when completed.
 *
 * @param ctx          Pointer to the container runtime context.
 * @param container_id The ID of the container to be stopped.
 * @param callback     Callback function to be invoked after stopping the container.
 * @return ocre_container_status_t Status of the stop operation.
 */
ocre_container_status_t CS_stop_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback);

/**
 * @brief Destroy a container.
 *
 * This function removes and destroys the specified container from the runtime.
 *
 * @param ctx          Pointer to the container runtime context.
 * @param container_id The ID of the container to be destroyed.
 * @param callback     Callback function to be invoked after destroying the container.
 * @return ocre_container_status_t Status of the container destruction.
 */
ocre_container_status_t CS_destroy_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback);

/**
 * @brief Restart a container.
 *
 * This function restarts a container by stopping and then running it again.
 *
 * @param ctx          Pointer to the container runtime context.
 * @param container_id The ID of the container to be restarted.
 * @param callback     Callback function to be invoked after restarting the container.
 * @return ocre_container_status_t Status of the container restart.
 */
ocre_container_status_t CS_restart_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback);

/**
 * @brief context initialization
 *
 * This function initialize the container runtime context.
 *
 * @param ctx          Pointer to the container runtime context.
 * @return POSIX style return if the container runtime context was initialized right 0 else -n
 */
int CS_ctx_init(ocre_cs_ctx *ctx);

#endif // OCRE_CS_SM_IMPL_H
