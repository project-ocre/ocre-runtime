/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wasm_export.h"

#ifndef OCRE_API_H
#define OCRE_API_H

#define OCRE_API_POSIX_BUF_SIZE 80

#ifndef OCRE_SYSTEM_NAME
#define OCRE_SYSTEM_NAME "Project Ocre"
#endif

struct _ocre_posix_utsname {
    char sysname[OCRE_API_POSIX_BUF_SIZE];
    char nodename[OCRE_API_POSIX_BUF_SIZE];
    char release[OCRE_API_POSIX_BUF_SIZE];
    char version[OCRE_API_POSIX_BUF_SIZE];
    char machine[OCRE_API_POSIX_BUF_SIZE];
    char domainname[OCRE_API_POSIX_BUF_SIZE];
};

int _ocre_posix_uname(wasm_exec_env_t exec_env, struct _ocre_posix_utsname *name);

extern NativeSymbol ocre_api_table[];
extern int ocre_api_table_size;
#endif
