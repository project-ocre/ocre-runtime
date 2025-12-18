/**
 * @file ocre.h
 *
 * Public Interface of Ocre Runtime Library.
 *
 * This header define the interfaces of general purpose container management in Ocre.
 */

#ifndef OCRE_H
#define OCRE_H

#include <stdbool.h>
#include <stddef.h>

#include <ocre/runtime/vtable.h>

/**
 * @mainpage Ocre Runtime Library Index
 * # Ocre Runtime Library
 *
 * Ocre is a runtime library for managing containers. It provides a set of APIs for creating, managing, and controlling
 * containers. It includes a WebAssembly runtime engine that can be used to run WebAssembly modules.
 *
 * This is just the auto-generated Doxygen documentation. Please refer to the full documentation and our website for
 * more information and detailed usage guidelines.
 *
 * # Public User API
 *
 * Used to control Ocre Library, manage contexts and containers.
 *
 * ## Ocre Library Control
 *
 * Used to initialize Ocre Runtime Library and create contexts.
 *
 * See ocre.h file documentation.
 *
 * ## Ocre Context Control: used to create and remove contexts.
 *
 * See ocre_context class for documentation.
 *
 * ## Ocre Container Control: used to manage containers.
 *
 * See ocre_container class for documentation.
 *
 * # Runtime Engine Virtual Table
 *
 * Used to add custom runtime engines to the Ocre Runtime Library.
 *
 * See ocre_runtime_vtable class for documentation.
 */

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
 * @brief Build configuration of the Ocre Library
 * @headerfile ocre.h <ocre/ocre.h>
 *
 * There should only be only one instance of this structure in the program. And it must be in constant read-only memory.
 * It is set at build-time and should only be read-only to the user.
 */
struct ocre_config {
	const char *version;	/**< Version of the Ocre Library */
	const char *commit_id;	/**< Commit ID of the build tree */
	const char *build_info; /**< Host build information */
	const char *build_date; /**< Build date */
};

/**
 * @brief The instance of configuration of the Ocre Library is constant and compiled-in.
 */
extern const struct ocre_config ocre_build_configuration;

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
 * @brief Initialize the Ocre Library and register runtime engines
 *
 * This function initializes the Ocre Library and register optional additional runtime engines. It must be called
 * exactly once before any other Ocre Library functions are used.
 *
 * @param vtable A pointer to a NULL-terminated Array of runtime engine vtables to register. Can be NULL.
 *
 * @return 0 on success, non-zero on failure
 */
int ocre_initialize(const struct ocre_runtime_vtable *const vtable[]);

/**
 * @brief Creates a new Ocre Context
 *
 * Creates a new Ocre Context with the specified working directory.
 *
 * Creating multiple Ocre Contexts is allowed, but each context must have its own working directory. Using the same
 * working directory for multiple contexts is considered an error and results in undefined behavior.
 *
 * @param workdir The working directory for the new context. Absolute path, NULL for default.
 *
 * @return a pointer to the newly created Ocre Context on success, NULL on failure
 */
struct ocre_context *ocre_create_context(const char *workdir);

/**
 * @brief Destroys an Ocre Context
 *
 * Destroys an Ocre Context. If there are any running containers, they will be killed and removed.
 *
 * @param context A pointer to the Ocre Context to destroy.
 *
 * @return 0 on success, non-zero on failure
 */
int ocre_destroy_context(struct ocre_context *context);

/**
 * @brief Deinitializes the Ocre Library
 *
 * Deinitializes the Ocre Library, destroying all contexts and containers.
 *
 * Any errors will be ignored.
 */
void ocre_deinitialize(void);

/**
 * @brief Creates a new container within the given context
 * @memberof ocre_context
 *
 * Creates a new container within the given context. The container will be created using the specified image, runtime,
 * container ID, and arguments. If the container is detached, it will be run in the background.
 *
 * @param context A pointer to the context in which to create the container
 * @param image The name of the image to use for the container
 * @param runtime The name of the runtime engine to use for the container
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
int ocre_context_get_num_containers(struct ocre_context *context);

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
int ocre_context_list_containers(struct ocre_context *context, struct ocre_container **containers, int max_size);

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

/**
 * @brief Check if a container or image ID is valid
 * @memberof ocre_context
 *
 * Checks if a container or image ID is valid. A valid container or image ID must not be NULL, empty, or start with a
 * dot '.'. It can only contain alphanumeric characters, dots, underscores, and hyphens.
 *
 * @param id A pointer to the container or image ID to check
 *
 * @return Zero if invalid, 1 if valid
 */
int ocre_is_valid_id(const char *id);

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

#endif /* OCRE_H */
