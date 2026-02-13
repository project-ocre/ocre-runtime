/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_COMMON_H
#define OCRE_COMMON_H

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
