/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_LIBRARY_H
#define OCRE_LIBRARY_H

#include <ocre/runtime/vtable.h>

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
 * working directory for multiple contexts will result in an error.
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

#endif /* OCRE_LIBRARY_H */
