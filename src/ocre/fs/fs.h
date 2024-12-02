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
#define PACKAGE_BASE_PATH_FMT OCRE_BASE_PATH "/manifest"
#define CONFIG_PATH           OCRE_BASE_PATH "/config/"

#define APP_RESOURCE_PATH_FMT     APP_RESOURCE_PATH "/%s.bin"
#define TEMP_RESOURCE_PATH_FMT    APP_RESOURCE_PATH "/%s.tmp"
#define PACKAGE_RESOURCE_PATH_FMT OCRE_BASE_PATH "packages/pkg-%d" // PACKAGE_BASE_PATH_FMT
#define CONFIG_PATH_FMT           CONFIG_PATH "/%s.bin"

#endif
