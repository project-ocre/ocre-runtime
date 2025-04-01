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
#include "../ocre_gpio/ocre_gpio.h"
#include "../container_messaging/messaging.h"

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
/**
 * @brief Pause execution for a specified time
 *
 * @param exec_env WASM execution environment
 * @param milliseconds Number of milliseconds to sleep
 * @return 0 on success
 */
int ocre_sleep(wasm_exec_env_t exec_env, int milliseconds) {
    k_msleep(milliseconds);
    return 0;
}

// Ocre Runtime API
NativeSymbol ocre_api_table[] = {
        {"uname", _ocre_posix_uname, "(*)i", NULL},

        {"ocre_sleep", ocre_sleep, "(i)i", NULL},

// Container Messaging API
#ifdef CONFIG_OCRE_CONTAINER_MESSAGING
        {"ocre_msg_system_init", ocre_msg_system_init, "()", NULL},
        {"ocre_publish_message", ocre_publish_message, "(***i)i", NULL},
        {"ocre_subscribe_message", ocre_subscribe_message, "(**)i", NULL},
#endif

// Sensor API
#ifdef CONFIG_OCRE_SENSORS
        // Sensor API
        {"ocre_sensors_init", ocre_sensors_init, "()i", NULL},
        {"ocre_sensors_discover", ocre_sensors_discover, "()i", NULL},
        {"ocre_sensors_open", ocre_sensors_open, "(i)i", NULL},
        {"ocre_sensors_get_handle", ocre_sensors_get_handle, "(i)i", NULL},
        {"ocre_sensors_get_channel_count", ocre_sensors_get_channel_count, "(i)i", NULL},
        {"ocre_sensors_get_channel_type", ocre_sensors_get_channel_type, "(ii)i", NULL},
        {"ocre_sensors_read", ocre_sensors_read, "(ii)i", NULL},
#endif
        // Timer API
        {"ocre_timer_create", ocre_timer_create, "(i)i", NULL},
        {"ocre_timer_start", ocre_timer_start, "(iii)i", NULL},
        {"ocre_timer_stop", ocre_timer_stop, "(i)i", NULL},
        {"ocre_timer_delete", ocre_timer_delete, "(i)i", NULL},
        {"ocre_timer_get_remaining", ocre_timer_get_remaining, "(i)i", NULL},
        {"ocre_timer_set_dispatcher", ocre_timer_set_dispatcher, "(i)v", NULL},

#ifdef CONFIG_OCRE_GPIO
        // GPIO API
        {"ocre_gpio_init", ocre_gpio_wasm_init, "()i", NULL},
        {"ocre_gpio_configure", ocre_gpio_wasm_configure, "(iii)i", NULL},
        {"ocre_gpio_pin_set", ocre_gpio_wasm_set, "(iii)i", NULL},
        {"ocre_gpio_pin_get", ocre_gpio_wasm_get, "(ii)i", NULL},
        {"ocre_gpio_pin_toggle", ocre_gpio_wasm_toggle, "(ii)i", NULL},
        {"ocre_gpio_register_callback", ocre_gpio_wasm_register_callback, "(ii)i", NULL},
        {"ocre_gpio_unregister_callback", ocre_gpio_wasm_unregister_callback, "(ii)i", NULL},
#endif
};

int ocre_api_table_size = sizeof(ocre_api_table) / sizeof(NativeSymbol);