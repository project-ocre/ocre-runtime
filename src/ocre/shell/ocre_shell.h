/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_SHELL_H
#define OCRE_SHELL_H

#include <zephyr/shell/shell.h>
#include "../src/ocre/ocre_container_runtime/ocre_container_runtime.h"

// Function declarations for `config` subcommand handlers
int cmd_ocre_run(const struct shell *shell, size_t argc, char **argv);
int cmd_ocre_stop(const struct shell *shell, size_t argc, char **argv);
int cmd_ocre_ps(const struct shell *shell, size_t argc, char **argv);

// Command registration function
void register_ocre_shell(ocre_cs_ctx *ctx);

#endif /* OCRE_SHELL_H */
