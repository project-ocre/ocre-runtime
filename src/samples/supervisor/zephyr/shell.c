#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <getopt.h>

#include <ocre/ocre.h>
#include <ocre/shell/shell.h>

extern struct ocre_context *ocre_global_context;

static int cmd_ocre_shell(const struct shell *sh, size_t argc, char **argv)
{
	return ocre_shell(ocre_global_context, argc, argv);
}

SHELL_CMD_REGISTER(ocre, NULL, "Ocre Runtime management", cmd_ocre_shell);
