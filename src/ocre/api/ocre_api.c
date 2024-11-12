/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <sys/utsname.h>
#include <string.h>
#include <sys/types.h>
#include <version.h>
#include <app_version.h>

#include "bh_platform.h"

#include "ocre_api.h"
#include "../ocre_timers/ocre_timer.h"
#include "../ocre_sensors/ocre_sensors.h"

int _ocre_posix_uname(wasm_exec_env_t exec_env, struct _ocre_posix_utsname *name)
{
    struct utsname info;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    if (!wasm_runtime_validate_native_addr(module_inst, name, sizeof(struct _ocre_posix_utsname)))
    {
        return -1;
    }

    if (uname(&info) != 0)
    {
        return -1;
    }

    memset(name, 0, sizeof(struct _ocre_posix_utsname));

    sprintf(name->sysname, "%s (%s)", OCRE_SYSTEM_NAME, info.sysname);
    sprintf(name->release, "%s (%s)", APP_VERSION_STRING, info.release);
    sprintf(name->version, "%s", info.version);

// ARM Processors are special cased as they are so popular
#ifdef CONFIG_ARM
#ifdef CONFIG_CPU_CORTEX_M0
    strcpy(name->machine, "ARM Cortex-M0");
#elif CONFIG_CPU_CORTEX_M3
    strcpy(name->machine, "ARM Cortex-M3");
#elif CONFIG_CPU_CORTEX_M4
    strcpy(name->machine, "ARM Cortex-M4");
#elif CONFIG_CPU_CORTEX_M7
    strcpy(name->machine, "ARM Cortex-M7");
#elif CONFIG_CPU_CORTEX_M33
    strcpy(name->machine, "ARM Cortex-M33");
#elif CONFIG_CPU_CORTEX_M23
    strcpy(name->machine, "ARM Cortex-M23");
#elif CONFIG_CPU_CORTEX_M55
    strcpy(name->machine, "ARM Cortex-M55");
#endif
#else
    // Other processors, use value returned from uname()
    strcpy(name->machine, info.machine);
#endif

    strcpy(name->nodename, info.nodename);

    return 0;
}

// Ocre Runtime API
NativeSymbol ocre_api_table[] = {{"uname", _ocre_posix_uname, "(*)i", NULL}};

int ocre_api_table_size = sizeof(ocre_api_table) / sizeof(NativeSymbol);