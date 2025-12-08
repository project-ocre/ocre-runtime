#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <getopt.h>

#include <ocre/ocre.h>
#include <ocre/shell/shell.h>

extern struct ocre_context *ocre_global_context;

static int cmd_ocre_shell(const struct shell *sh, size_t argc, char **argv)
{
	// getopt_init();

	// volatile struct getopt_state *state = getopt_state_get();

	// optreset = 1;

	return ocre_shell(ocre_global_context, argc, argv);
}

SHELL_CMD_REGISTER(ocre, NULL, "OCRE management", cmd_ocre_shell);
