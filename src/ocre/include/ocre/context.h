/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_CONTEXT_H
#define OCRE_CONTEXT_H

#include <stdbool.h>

struct ocre_container;

/**
 * @class ocre_context
 * @headerfile ocre.h <ocre/ocre.h>
 * @brief Ocre Context
 *
 * The context is created by ocre_create_context() and destroyed by ocre_destroy_context(). It is used to interact with
 * Ocre Library and passed around as pointers.
 *
 * The Ocre Context is a manager for a set of containers on their own runtime engines.
 */
struct ocre_context;

/**
 * @brief Container arguments
 * @headerfile ocre.h <ocre/ocre.h>
 *
 * A structure representing container arguments. These are passed to the runtime engine to define
 * the container's behavior.
 *
 * The parameters are pointers to a NULL-terminated arrays of pointers standard (zero-terminated) character C strings.
 */
struct ocre_container_args {
	/** @brief arguments to the container
	 *
	 * It is passed as parameters to the container's entry point. This is a NULL-terminated array of pointers to
	 * standard (zero-terminated) character C strings.
	 *
	 * Should not include the container's name. Any value here, starting from argv[0] will be passed to the
	 * container's entry point as argv[1] and so on.
	 *
	 * Example:
	 * @code
	 * const char *argv[] = {
	 *     "arg1",
	 *     "arg2",
	 *     NULL
	 * };
	 * @endcode
	 */
	const char **argv;

	/** @brief environment variables to the container
	 *
	 * It is passed as environment variables to the container's entry point.
	 *
	 * The values should be in the format "VAR=value".
	 *
	 * Example:
	 * @code
	 * const char *envp[] = {
	 *     "VAR1=value1",
	 *     "VAR2=value2",
	 *     NULL
	 * };
	 * @endcode
	 */
	const char **envp;

	/** @brief enabled permissions and features for the container
	 *
	 * It is used to enable specific container features and permissions. The possible values depends on the
	 * implementation. Check the full documentation for more details.
	 *
	 * This is a NULL-terminated array of pointers to standard (zero-terminated) character C strings.
	 *
	 * Example:
	 * @code
	 * const char *capabilities[] = {
	 *     "filesystem",
	 *     "networking",
	 *     "ocre:api",
	 *     NULL
	 * };
	 * @endcode
	 */
	const char **capabilities;

	/** @brief Virtual mounts for the container.
	 *
	 * It is used to expose files. The format is in the form of "source:destination".
	 *
	 * This is a NULL-terminated array of pointers to standard (zero-terminated) character C strings and is in the
	 * form "<source>:<destination>"
	 *
	 * The current implementation supports the following:
	 * - "/absolute/path/to/dir/on/host:/absolute/path/in/container" to mount a directory from the host into the
	 * container.
	 *
	 * Note: relative path, volumes and file-mounts are not yet supported.
	 *
	 * The Ocre Process should have the necessary permissions to access the source directory.
	 *
	 * Example:
	 * @code
	 * const char *mounts[] = {
	 *     "/dev:/dev",
	 *     NULL
	 * };
	 * @endcode
	 */
	const char **mounts;
};

/**
 * @brief Creates a new container within the given context
 * @memberof ocre_context
 *
 * Creates a new container within the given context. The container will be created using the specified image, runtime,
 * container ID, and arguments. If the container is detached, it will be run in the background.
 *
 * @param context A pointer to the context in which to create the container
 * @param image The name of the image to use for the container
 * @param runtime The name of the runtime engine to use for the container. Can be NULL, in which case Ocre will attempt
 * to autoatically determine the best runtime for the image.
 * @param container_id The ID to assign to the container. Can be NULL, in which case a random ID will be generated
 * @param detached Whether the container should be detached (run in the background) or not
 * @param arguments The container arguments to pass to the container. Can be NULL
 *
 * @return A pointer to the newly created container, or NULL on failure
 */
struct ocre_container *ocre_context_create_container(struct ocre_context *context, const char *image,
						     const char *const runtime, const char *container_id, bool detached,
						     const struct ocre_container_args *arguments);

/**
 * @brief Get a container by its ID
 * @memberof ocre_context
 *
 * @param context A pointer to the context in which to get the container.
 * @param id The ID of the container to get.
 *
 * @return A pointer to the container, or NULL if not found
 */
struct ocre_container *ocre_context_get_container_by_id(struct ocre_context *context, const char *id);

/**
 * @brief Remove a container
 * @memberof ocre_context
 *
 * @param context A pointer to the context in which to remove the container.
 * @param container A pointer to the container to remove.
 *
 * @return 0 on success, non-zero on failure
 */
int ocre_context_remove_container(struct ocre_context *context, struct ocre_container *container);

/**
 * @brief Get the number of containers in the context
 * @memberof ocre_context
 *
 * @param context A pointer to the context in which to get the number of containers
 *
 * @return The number of containers in the context
 */
int ocre_context_get_container_count(struct ocre_context *context);

/**
 * @brief List containers in the context
 * @memberof ocre_context
 *
 * This function will populate the provided array with pointers to the containers, up to the maximum size.
 *
 * @param context A pointer to the context in which to list containers
 * @param[out] containers A pointer to an array of pointers to containers
 * @param[in] max_size The maximum size of the array
 *
 * @return The number of containers listed, negative on failure
 */
int ocre_context_get_containers(struct ocre_context *context, struct ocre_container **containers, int max_size);

/**
 * @brief Get the working directory of the context
 * @memberof ocre_context
 *
 * Returns a pointer to a C string containing the working directory. This string is owned by the context and is valid
 * until the context is destroyed. Should not be modified or freed by the caller.
 *
 * @param context A pointer to the context in which to get the working directory
 *
 * @return A pointer to the working directory string, or NULL on failure
 */
const char *ocre_context_get_working_directory(const struct ocre_context *context);

#endif /* OCRE_CONTEXT_H */
