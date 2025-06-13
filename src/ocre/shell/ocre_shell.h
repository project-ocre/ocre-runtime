#include <zephyr/shell/shell.h>
#include "../src/ocre/ocre_container_runtime/ocre_container_runtime.h"

// Function declarations for `config` subcommand handlers
int cmd_ocre_run(const struct shell *shell, size_t argc, char **argv);
int cmd_ocre_stop(const struct shell *shell, size_t argc, char **argv);
int cmd_ocre_ps(const struct shell *shell, size_t argc, char **argv);

// Command registration function
void register_ocre_shell(ocre_cs_ctx *ctx);