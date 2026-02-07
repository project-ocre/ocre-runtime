/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_CONTAINER_H
#define OCRE_CONTAINER_H

#include <stdbool.h>

/**
 * @brief The possible status of a container
 *
 * This enum represents the possible status of a container. The container status are all side-effects from the
 * container's lifecycle.
 *
 * The status OCRE_CONTAINER_STATUS_EXITED is a transient status that indicates the container has exited but we did not
 * get the exit code yet. Upon calling ocre_container_wait(), the status will be updated to
 * OCRE_CONTAINER_STATUS_STOPPED in case it was OCRE_CONTAINER_STATUS_EXITED.
 */
typedef enum {
	OCRE_CONTAINER_STATUS_UNKNOWN = 0, /**< Status is unknown */
	OCRE_CONTAINER_STATUS_CREATED,	   /**< Container has been created. */
	OCRE_CONTAINER_STATUS_RUNNING,	   /**< Container is currently running. */
	OCRE_CONTAINER_STATUS_PAUSED,	   /**< Container is currently paused. */
	OCRE_CONTAINER_STATUS_EXITED,	   /**< Container has exited but we did not get the exit code yet. */
	OCRE_CONTAINER_STATUS_STOPPED,	   /**< Container has been stopped. */
	OCRE_CONTAINER_STATUS_ERROR,	   /**< An error occurred with the container. */
} ocre_container_status_t;

/**
 * @class ocre_container
 * @headerfile ocre.h <ocre/ocre.h>
 * @brief Ocre Container
 *
 * An opaque structure representing the container. It should be used to interact with Ocre Library and passed around as
 * pointers.
 *
 * An Ocre Container is an application running on a runtime engine.
 */
struct ocre_container;

/**
 * @brief Start a container
 * @memberof ocre_container
 *
 * Starts a container in the context. The container must be in the CREATED or STOPPED states. Calling this function on a
 * container on other states will result in error.
 *
 * @param container A pointer to the container to start
 *
 * @return Zero on success, non-zero on failure
 */
int ocre_container_start(struct ocre_container *container);

/**
 * @brief Get the status of a container
 * @memberof ocre_container
 *
 * Returns the status of a container in the context.
 *
 * @param container A pointer to the container to get the status of
 *
 * @return The status of the container. UNKNOWN on failure
 */
ocre_container_status_t ocre_container_get_status(struct ocre_container *container);

/**
 * @brief Get the ID of a container
 * @memberof ocre_container
 *
 * Returns a pointer to a C string containing the ID of the container. This string is owned by the container and is
 * valid until the container is destroyed. Should not be modified or freed by the caller.
 *
 * @param container A pointer to the container to get the ID of
 *
 * @return A pointer to the ID string, or NULL on failure
 */
const char *ocre_container_get_id(const struct ocre_container *container);

/**
 * @brief Get the image of a container
 * @memberof ocre_container
 *
 * Returns a pointer to a C string containing the image of the container. This string is owned by the container and is
 * valid until the container is destroyed. Should not be modified or freed by the caller.
 *
 * @param container A pointer to the container to get the image of
 *
 * @return A pointer to the image string, or NULL on failure
 */
const char *ocre_container_get_image(const struct ocre_container *container);

/**
 * @brief Pause a container
 * @memberof ocre_container
 *
 * Pauses a container in the context. The container must be in the RUNNING state, otherwise it will fail.
 *
 * Note: This is currently not supported in WAMR containers.
 *
 * @param container A pointer to the container to pause
 *
 * @return Zero on success, non-zero on failure
 */
int ocre_container_pause(struct ocre_container *container);

/**
 * @brief Unpause a container
 * @memberof ocre_container
 *
 * Unpauses a container in the context. The container must be in the PAUSED state, otherwise it will fail.
 *
 * Note: This is currently not supported in WAMR containers.
 *
 * @param container A pointer to the container to unpause
 *
 * @return Zero on success, non-zero on failure
 */
int ocre_container_unpause(struct ocre_container *container);

/**
 * @brief Gracefully stop a container
 * @memberof ocre_container
 *
 * Stops a container in the context. The container must be in the RUNNING state, otherwise it will fail.
 *
 * The operation will signal the container to stop, and after a grace period, it will be forcefully terminated if not
 * exited.
 *
 * Note: This is currently not supported in WAMR containers.
 *
 * @param container A pointer to the container to stop
 *
 * @return Zero on success, non-zero on failure
 */
int ocre_container_stop(struct ocre_container *container);

/**
 * @brief Forcefully terminate a container
 * @memberof ocre_container
 *
 * Terminates a container in the context. The container must be in the RUNNING state, otherwise it will fail.
 *
 * @param container A pointer to the container to terminate
 *
 * @return Zero on success, non-zero on failure
 */
int ocre_container_kill(struct ocre_container *container);

/**
 * @brief Wait for a container to exit
 * @memberof ocre_container
 *
 * If the container is STOPPED, returns the exit status of the container.
 *
 * If the container is RUNNING or PAUSED, waits for the container to exit.
 *
 * If the container is in other states, it will fail.
 *
 * @param container A pointer to the container to wait for
 * @param[out] status A pointer to store the exit status of the container. Can be NULL
 *
 * @return Zero on success, non-zero on failure
 */
int ocre_container_wait(struct ocre_container *container, int *status);

/**
 * @brief Get detached mode
 * @memberof ocre_container
 *
 * Detached containers run on background.
 *
 * @param container A pointer to the container to terminate
 *
 * @return true if container is detached, false otherwise
 */
bool ocre_container_is_detached(struct ocre_container *container);

#endif /* OCRE_CONTAINER_H */
