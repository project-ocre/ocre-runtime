/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_print.h"

#ifdef __ZEPHYR__
#include <zephyr/sys/printk.h>
#else
#include <stdio.h>
#endif

int ocre_print_wasm(wasm_exec_env_t exec_env, const char *msg)
{
	if (!msg) {
		return -1;
	}

#ifdef __ZEPHYR__
	printk("%s\n", msg);
#else
	printf("%s\n", msg);
#endif

	return 0;
}

void ocre_abort_wasm(wasm_exec_env_t exec_env, const char *message, const char *file_name, uint32_t line_number,
		     uint32_t column_number)
{
#ifdef __ZEPHYR__
	printk("[wasm abort] %s (%s:%u:%u)\n", message ? message : "", file_name ? file_name : "", line_number,
	       column_number);
#else
	printf("[wasm abort] %s (%s:%u:%u)\n", message ? message : "", file_name ? file_name : "", line_number,
	       column_number);
#endif

	wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);

	if (module_inst) {
		wasm_runtime_set_exception(module_inst, "wasm abort");
	}
}
