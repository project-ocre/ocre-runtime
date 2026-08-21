/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_PRINT_H
#define OCRE_PRINT_H

#include <stdint.h>
#include <wasm_export.h>

/**
 * @brief Print a message from a WASM container to the host console.
 *
 * Portable across every platform Ocre runs on: uses Zephyr's printk() when
 * built for Zephyr, and printf() otherwise (e.g. the Linux/posix runtime).
 *
 * @param exec_env WASM execution environment.
 * @param msg Null-terminated string to print.
 *
 * @return 0 on success, negative error code on failure.
 */
int ocre_print_wasm(wasm_exec_env_t exec_env, const char *msg);

/**
 * @brief Backs the "env.abort" import that AssemblyScript-compiled modules
 * expect (used for reporting failed runtime checks like array bounds).
 *
 * Prints the failure and raises a WASM exception, cleanly stopping the
 * calling container's execution (not a host crash).
 */
void ocre_abort_wasm(wasm_exec_env_t exec_env, const char *message, const char *file_name, uint32_t line_number,
		     uint32_t column_number);

#endif /* OCRE_PRINT_H */
