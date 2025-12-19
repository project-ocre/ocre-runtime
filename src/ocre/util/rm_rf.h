/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RM_RF_H
#define RM_RF_H

/**
 * Remove a file or directory recursively.
 *
 * This function removes the specified path and all its contents recursively.
 * It uses a stack-based approach to traverse directories without function recursion.
 *
 * @param path The path to the file or directory to remove
 * @return 0 on success, -1 on error (errno is set appropriately)
 *
 * Note: This function will attempt to remove as many files/directories as possible
 * even if some operations fail. The return value indicates if any errors occurred.
 */
int rm_rf(const char *path);

#endif /* RM_RF_H */
