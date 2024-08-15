/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wasm_export.h"

#ifndef OCRE_API_H
#define OCRE_API_H

#ifndef OCRE_SYSTEM_NAME
#define OCRE_SYSTEM_NAME "Project Ocre"
#endif

struct _ocre_posix_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

int _ocre_posix_uname(wasm_exec_env_t exec_env, struct _ocre_posix_utsname *name);

extern NativeSymbol ocre_api_table[];
extern int ocre_api_table_size;
#endif