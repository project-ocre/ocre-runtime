/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_FILESYSTEM_H
#define OCRE_FILESYSTEM_H

void ocre_app_storage_init();

// clang-format off
#define FS_MOUNT_POINT "/lfs"
#define OCRE_BASE_PATH FS_MOUNT_POINT "/ocre"

#define APP_RESOURCE_PATH     OCRE_BASE_PATH "/images"
#define PACKAGE_BASE_PATH     OCRE_BASE_PATH "/manifests"
#define CONFIG_PATH           OCRE_BASE_PATH "/config/"

#endif
