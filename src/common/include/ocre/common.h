/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_COMMON_H
#define OCRE_COMMON_H

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
 * @brief Check if a container or image names is valid
 * @memberof ocre_context
 *
 * Checks if a container or image name is valid. A valid container or image name must not be NULL, empty, or start with
 * a dot '.'. It can only contain alphanumeric characters, dots, underscores, and hyphens.
 *
 * @param id A pointer to the container or image ID to check
 *
 * @return Zero if invalid, 1 if valid
 */
int ocre_is_valid_name(const char *id);

#endif /* OCRE_COMMON_H */
