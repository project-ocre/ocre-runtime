/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <sys/utsname.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "ocre_api.h"
#include "utils/strlcat.h"
#include "ocre_print/ocre_print.h"

#include <ocre/platform/config.h>

#ifdef CONFIG_OCRE_HTTP_SERVER
#include "ocre_http/ocre_http.h"
#endif

#ifdef CONFIG_OCRE_HTTP_CLIENT
#include "ocre_http_client/ocre_http_client.h"
#endif

#ifdef CONFIG_OCRE_TIMER
#include "ocre_timers/ocre_timer.h"
#endif

#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS) ||                         \
	defined(CONFIG_OCRE_CONTAINER_MESSAGING)
#include "ocre_common.h"
#endif

#ifdef CONFIG_OCRE_SENSORS
#include "ocre_sensors/ocre_sensors.h"
#endif

#ifdef CONFIG_OCRE_GPIO
#include "ocre_gpio/ocre_gpio.h"
#endif

#ifdef CONFIG_OCRE_ADC
#include "ocre_adc/ocre_adc.h"
#endif

#ifdef CONFIG_OCRE_CONTAINER_MESSAGING
#include "ocre_messaging/ocre_messaging.h"
#endif

#ifndef APP_VERSION_STRING
#define APP_VERSION_STRING "1.0.0"
#endif

int _ocre_posix_uname(wasm_exec_env_t exec_env, struct _ocre_posix_utsname *name)
{
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

int ocre_sleep(wasm_exec_env_t exec_env, int milliseconds)
{
	if (milliseconds <= 0) {
		return 0;
	}

	/* POSIX usleep() rejects any value >= 1 second (returns -1/EINVAL
	 * instantly instead of sleeping), so chunk the delay into sub-second
	 * calls to support arbitrary durations. */
	while (milliseconds >= 1000) {
		usleep(999000);
		milliseconds -= 999;
	}

	if (milliseconds > 0) {
		usleep((unsigned int)milliseconds * 1000);
	}

	return 0;
}

// Ocre Runtime API
NativeSymbol ocre_api_table[] = {
	{"uname", _ocre_posix_uname, "(*)i", NULL},
	{"ocre_sleep", ocre_sleep, "(i)i", NULL},
	{"ocre_print", ocre_print_wasm, "($)i", NULL},
	/* Backs AssemblyScript-compiled modules' "env.abort" import. Void
	 * return: no trailing signature char (WAMR only checks a return char
	 * when the wasm import itself declares a result). */
	{"abort", ocre_abort_wasm, "($$ii)", NULL},
#ifdef CONFIG_OCRE_HTTP_SERVER
	{"ocre_http_get_method", ocre_http_get_method_wasm, "(i)i", NULL},
	{"ocre_http_get_path", ocre_http_get_path_wasm, "(i*~)i", NULL},
	{"ocre_http_get_query", ocre_http_get_query_wasm, "(i*~)i", NULL},
	{"ocre_http_get_header", ocre_http_get_header_wasm, "(i$*~)i", NULL},
	{"ocre_http_get_body", ocre_http_get_body_wasm, "(i*~)i", NULL},
	{"ocre_http_respond", ocre_http_respond_wasm, "(ii$$)i", NULL},
#endif
#ifdef CONFIG_OCRE_HTTP_CLIENT
	{"ocre_http_get", ocre_http_client_get_wasm, "($i$*~)i", NULL},
#endif
#if defined(CONFIG_OCRE_TIMER) || defined(CONFIG_OCRE_GPIO) || defined(CONFIG_OCRE_SENSORS) ||                         \
	defined(CONFIG_OCRE_CONTAINER_MESSAGING)
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
// ADC API
#ifdef CONFIG_OCRE_ADC
	{"ocre_adc_init", ocre_adc_wasm_init, "()i", NULL},
	{"ocre_adc_read_by_name", ocre_adc_wasm_read_by_name, "($)i", NULL},
#endif
};

int ocre_api_table_size = sizeof(ocre_api_table) / sizeof(NativeSymbol);
