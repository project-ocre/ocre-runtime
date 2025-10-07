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

#include "ocre_core_external.h"

#include "bh_platform.h"

#include "ocre_api.h"

#ifdef CONFIG_OCRE_TIMER
#include "../ocre_timers/ocre_timer.h"
#endif

#include "ocre/utils/utils.h"

#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS) || defined(CONFIG_OCRE_CONTAINER_MESSAGING)
#include "ocre_common.h"
#endif 

#ifdef CONFIG_OCRE_SENSORS
#include "../ocre_sensors/ocre_sensors.h"
#endif

#ifdef CONFIG_OCRE_GPIO
#include "../ocre_gpio/ocre_gpio.h"
#endif

#ifdef CONFIG_OCRE_CONTAINER_MESSAGING
#include "../ocre_messaging/ocre_messaging.h"
#endif

#ifdef CONFIG_OCRE_DISPLAY
#include "../ocre_display/ocre_display.h"
#endif

int _ocre_posix_uname(wasm_exec_env_t exec_env, struct _ocre_posix_utsname *name) {
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        return -1;
    }
    if (!wasm_runtime_validate_native_addr(module_inst, name, sizeof(struct _ocre_posix_utsname))) {
        return -1;
    }

    struct utsname info;
    if (uname(&info) != 0) {
        return -1;
    }

    memset(name, 0, sizeof(struct _ocre_posix_utsname));

    snprintf(name->sysname, OCRE_API_POSIX_BUF_SIZE, "%s (%s)", OCRE_SYSTEM_NAME, info.sysname);
    snprintf(name->release, OCRE_API_POSIX_BUF_SIZE, "%s (%s)", APP_VERSION_STRING, info.release);
    snprintf(name->version, OCRE_API_POSIX_BUF_SIZE, "%s", info.version);

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
    strlcat(name->machine, info.machine, OCRE_API_POSIX_BUF_SIZE);
#endif

    strlcat(name->nodename, info.nodename, OCRE_API_POSIX_BUF_SIZE);

    return 0;
}

int ocre_sleep(wasm_exec_env_t exec_env, int milliseconds) {
    core_sleep_ms(milliseconds);
    return 0;
}

// Ocre Runtime API
NativeSymbol ocre_api_table[] = {
        {"uname", _ocre_posix_uname, "(*)i", NULL},
        {"ocre_sleep", ocre_sleep, "(i)i", NULL},
#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS) || defined(CONFIG_OCRE_CONTAINER_MESSAGING)
        {"ocre_get_event", ocre_get_event, "(iiiiii)i", NULL},
        {"ocre_register_dispatcher", ocre_register_dispatcher, "(i$)i", NULL},
#endif 
// Container Messaging API
#ifdef CONFIG_OCRE_CONTAINER_MESSAGING
        {"ocre_publish_message", ocre_messaging_publish, "(***i)i", NULL},
        {"ocre_subscribe_message", ocre_messaging_subscribe, "(*)i", NULL},
        {"ocre_messaging_free_module_event_data", ocre_messaging_free_module_event_data, "(iii)i", NULL},
#endif
// Sensor API
#ifdef CONFIG_OCRE_SENSORS
        {"ocre_sensors_init", ocre_sensors_init, "()i", NULL},
        {"ocre_sensors_discover", ocre_sensors_discover, "()i", NULL},
        {"ocre_sensors_open", ocre_sensors_open, "(i)i", NULL},
        {"ocre_sensors_get_handle", ocre_sensors_get_handle, "(i)i", NULL},
        {"ocre_sensors_get_channel_count", ocre_sensors_get_channel_count, "(i)i", NULL},
        {"ocre_sensors_get_channel_type", ocre_sensors_get_channel_type, "(ii)i", NULL},
        {"ocre_sensors_read", ocre_sensors_read, "(ii)F", NULL},
        {"ocre_sensors_open_by_name", ocre_sensors_open_by_name, "($)i", NULL},
        {"ocre_sensors_get_handle_by_name", ocre_sensors_get_handle_by_name, "($)i", NULL},
        {"ocre_sensors_get_channel_count_by_name", ocre_sensors_get_channel_count_by_name, "($)i", NULL},
        {"ocre_sensors_get_channel_type_by_name", ocre_sensors_get_channel_type_by_name, "($i)i", NULL},
        {"ocre_sensors_read_by_name", ocre_sensors_read_by_name, "($i)F", NULL},
        {"ocre_sensors_get_list", ocre_sensors_get_list, "($i)i", NULL},
#endif
// Timer API
#ifdef CONFIG_OCRE_TIMER
        {"ocre_timer_create", ocre_timer_create, "(i)i", NULL},
        {"ocre_timer_start", ocre_timer_start, "(iii)i", NULL},
        {"ocre_timer_stop", ocre_timer_stop, "(i)i", NULL},
        {"ocre_timer_delete", ocre_timer_delete, "(i)i", NULL},
        {"ocre_timer_get_remaining", ocre_timer_get_remaining, "(i)i", NULL},
#endif
// GPIO API
#ifdef CONFIG_OCRE_GPIO
        {"ocre_gpio_init", ocre_gpio_wasm_init, "()i", NULL},
        {"ocre_gpio_configure", ocre_gpio_wasm_configure, "(iii)i", NULL},
        {"ocre_gpio_pin_set", ocre_gpio_wasm_set, "(iii)i", NULL},
        {"ocre_gpio_pin_get", ocre_gpio_wasm_get, "(ii)i", NULL},
        {"ocre_gpio_pin_toggle", ocre_gpio_wasm_toggle, "(ii)i", NULL},
        {"ocre_gpio_register_callback", ocre_gpio_wasm_register_callback, "(ii)i", NULL},
        {"ocre_gpio_unregister_callback", ocre_gpio_wasm_unregister_callback, "(ii)i", NULL},

        {"ocre_gpio_configure_by_name", ocre_gpio_wasm_configure_by_name, "($i)i", NULL},
        {"ocre_gpio_set_by_name", ocre_gpio_wasm_set_by_name, "($i)i", NULL},
        {"ocre_gpio_get_by_name", ocre_gpio_wasm_get_by_name, "($)i", NULL},
        {"ocre_gpio_toggle_by_name", ocre_gpio_wasm_toggle_by_name, "($)i", NULL},
        {"ocre_gpio_register_callback_by_name", ocre_gpio_wasm_register_callback_by_name, "($)i", NULL},
        {"ocre_gpio_unregister_callback_by_name", ocre_gpio_wasm_unregister_callback_by_name, "($)i", NULL},
#endif

#ifdef CONFIG_OCRE_DISPLAY
        {"display_init",                ocre_display_init,              "()i",      NULL},        
        {"display_get_capabilities",    ocre_display_get_capabilities,  "(****)i",  NULL},
        {"display_input_read",          ocre_display_input_read,        "(****)",   NULL},
        {"display_flush",               ocre_display_flush,             "(iiii*)",  NULL},
        {"time_get_ms",                 ocre_time_get_ms,               "()i",      NULL}
#endif
};

int ocre_api_table_size = sizeof(ocre_api_table) / sizeof(NativeSymbol);
