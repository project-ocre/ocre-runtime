/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE

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

int _ocre_posix_uname(wasm_exec_env_t exec_env, struct _ocre_posix_utsname *name) {
    struct utsname info;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    if (!wasm_runtime_validate_native_addr(module_inst, name, sizeof(struct _ocre_posix_utsname))) {
        return -1;
    }

    if (uname(&info) != 0) {
        return -1;
    }

    memset(name, 0, sizeof(struct _ocre_posix_utsname));

    snprintf(name->sysname, OCRE_API_POSIX_BUF_SIZE, "%s (%s)", OCRE_SYSTEM_NAME, info.sysname);
    snprintf(name->release, OCRE_API_POSIX_BUF_SIZE, "%s (%s)", APP_VERSION_STRING, info.release);
    snprintf(name->version, OCRE_API_POSIX_BUF_SIZE, "%s", info.version);

// ARM Processors are special cased as they are so popular
#ifdef CONFIG_ARM
#ifdef CONFIG_CPU_CORTEX_M0
    strlcat(name->machine, "ARM Cortex-M0", OCRE_API_POSIX_BUF_SIZE);
#elif CONFIG_CPU_CORTEX_M3
    strlcat(name->machine, "ARM Cortex-M3", OCRE_API_POSIX_BUF_SIZE);
#elif CONFIG_CPU_CORTEX_M4
    strlcat(name->machine, "ARM Cortex-M4", OCRE_API_POSIX_BUF_SIZE);
#elif CONFIG_CPU_CORTEX_M7
    strlcat(name->machine, "ARM Cortex-M7", OCRE_API_POSIX_BUF_SIZE);
#elif CONFIG_CPU_CORTEX_M33
    strlcat(name->machine, "ARM Cortex-M33", OCRE_API_POSIX_BUF_SIZE);
#elif CONFIG_CPU_CORTEX_M23
    strlcat(name->machine, "ARM Cortex-M23", OCRE_API_POSIX_BUF_SIZE);
#elif CONFIG_CPU_CORTEX_M55
    strlcat(name->machine, "ARM Cortex-M55", OCRE_API_POSIX_BUF_SIZE);
#endif
#else
    // Other processors, use value returned from uname()
    strlcat(name->machine, info.machine, OCRE_API_POSIX_BUF_SIZE);
#endif

    strlcat(name->nodename, info.nodename, OCRE_API_POSIX_BUF_SIZE);

    return 0;
}

// Ocre Runtime API
NativeSymbol ocre_api_table[] = {
        {"uname", _ocre_posix_uname, "(*)i", NULL},
        {"ocre_sensors_init", ocre_sensors_init, "()i", NULL},
        {"ocre_sensors_discover_sensors", ocre_sensors_discover_sensors, "(**i)i", NULL},
        {"ocre_sensors_open_channel", ocre_sensors_open_channel, "(*i)i", NULL},
        {"sensor_read_sample", sensor_read_sample, "(*i)i", NULL},
        {"sensor_get_channel", sensor_get_channel, "(ii)i", NULL},
        {"ocre_sensors_set_trigger", ocre_sensors_set_trigger, "(*ii*i)i", NULL},
        {"ocre_sensors_clear_trigger", ocre_sensors_clear_trigger, "(*ii)i", NULL},
        {"ocre_sensors_cleanup", ocre_sensors_cleanup, "()i", NULL},
};

int ocre_api_table_size = sizeof(ocre_api_table) / sizeof(NativeSymbol);