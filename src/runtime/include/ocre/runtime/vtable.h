#ifndef OCRE_RUNTIME_VTABLE_H
#define OCRE_RUNTIME_VTABLE_H

#include <stddef.h>
#include <pthread.h>

/**
 * @brief Runtime Engine Virtual Table
 * @headerfile vtable.h <ocre/runtime/vtable.h>
 *
 * This is the interface between the ocre container and the runtime engine.
 *
 * These functions are guaranteed to be operated according to the Ocre Container lifecycle specification.
 *
 * Please refer to the full documentation for more details.
 *
 * Refer to the wamr engine implementation for an example of how to implement this interface.
 */
struct ocre_runtime_vtable {
	/**
	 * @brief Runtime Engine Name
	 *
	 * This is the name of the runtime engine. Should be a zero-terminated string.
	 *
	 * It should be unique among all registered runtime engines. Please refer to the full documentation for the list
	 * of assigned runtime engine names.
	 */
	const char *const runtime_name;

	/**
	 * @brief Initialize the runtime engine
	 *
	 * This function is called when the runtime engine is initialized. It is called only one on start and before any
	 * other functions are called.
	 *
	 * @return 0 on success, non-zero on failure
	 */
	int (*init)(void);

	/**
	 * @brief Deinitialize the runtime engine
	 *
	 * This function is called when the runtime engine is deinitialized.
	 *
	 * @return 0 on success, non-zero on failure
	 */
	int (*deinit)(void);

	/**
	 * @brief Create a new runtime instance
	 *
	 * This function is called when a new runtime instance is created.
	 *
	 * This function will return an opaque pointer to the context of the runtime instance. It can be anything
	 * managed internally by the runtime. It cannot be NULL, as it will indicate error. This value is passed as a
	 * parameter to the other functions in this interface.
	 *
	 * @param img_path Absolute path to the image file
	 * @param workdir Working directory for the runtime instance. Can be NULL.
	 * @param stack_size Stack size for the runtime instance
	 * @param heap_size Heap size for the runtime instance
	 * @param capabilities A NULL-terminated array of capabilities for the runtime instance
	 * @param argv A NULL-terminated array of command line arguments to be passed to the container
	 * @param envp A NULL-terminated array of environment variables to be passed to the container
	 * @param mounts Mount points for the runtime instance
	 *
	 * @return Pointer to the runtime context on success, NULL on failure
	 */
	void *(*create)(const char *img_path, const char *workdir, size_t stack_size, size_t heap_size,
			const char **capabilities, const char **argv, const char **envp, const char **mounts);

	/**
	 * @brief Destroy a runtime instance
	 *
	 * This function is called when a runtime instance is destroyed. This is guaranteed to be called only when the
	 * container should not be running or paused.
	 *
	 * @param runtime_context Pointer to the runtime context
	 * @return 0 on success, non-zero on failure
	 */
	int (*destroy)(void *runtime_context);

	/**
	 * @brief Thread execute function for the runtime.
	 *
	 * This function is called inside a thread when the container is started. It should execute the container's main
	 * function, block and eventually return the exit code.
	 *
	 * @param runtime_context Pointer to the runtime context returned by create
	 * @param cond Pointer to the conditional variable to signal when the instance is ready to be killed
	 *
	 * @return status code of the execution: 0 on success, non-zero on failure
	 */

	int (*thread_execute)(void *runtime_context, pthread_cond_t *cond);

	/**
	 * @brief Stop a runtime instance
	 *
	 * This function is called when a runtime instance is requested to stop. This is guaranteed to be called only
	 * when the container is running or paused.
	 *
	 * @param runtime_context Pointer to the runtime context
	 * @return 0 on success, non-zero on failure
	 */
	int (*stop)(void *runtime_context);

	/**
	 * @brief Kill a runtime instance
	 *
	 * This function is called when a runtime instance is requested to be killed. This is guaranteed to be called
	 * only when the container is running. This should terminate the container immediately.
	 *
	 * @param runtime_context Pointer to the runtime context
	 * @return 0 on success, non-zero on failure
	 */
	int (*kill)(void *runtime_context);

	/**
	 * @brief Pause a runtime instance
	 *
	 * This function is called when a runtime instance is requested to be paused. This is guaranteed to be called
	 * only when the container is running.
	 *
	 * @param runtime_context Pointer to the runtime context
	 * @return 0 on success, non-zero on failure
	 */
	int (*pause)(void *runtime_context);

	/**
	 * @brief Unpause a runtime instance
	 *
	 * This function is called when a runtime instance is requested to be unpaused. This is guaranteed to be called
	 * only when the container is paused.
	 *
	 * @param runtime_context Pointer to the runtime context
	 * @return 0 on success, non-zero on failure
	 */
	int (*unpause)(void *runtime_context);
};

#endif /* OCRE_RUNTIME_VTABLE_H */
