#include <stdio.h>
#include <string.h>

#include "command.h"

#include "image/ls.h"
#include "image/rm.h"
#include "image/pull.h"

static int print_usage(struct ocre_context *ctx, const char *argv0, int argc, char **argv)
{
	fprintf(stderr, "Usage: %s image <COMMAND>\n", argv0);

	fprintf(stderr, "\nCommands:\n");
	fprintf(stderr, "  ls        List images\n");
	fprintf(stderr, "  pull      Pull an image\n");
	fprintf(stderr, "  rm        Remove an image\n");
	return -1;
}

static const struct ocre_command commands[] = {
	{"help", print_usage},
	{"ls", cmd_image_ls},
	{"pull", cmd_image_pull},
	{"rm", cmd_image_rm},
};

int cmd_image(struct ocre_context *ctx, const char *argv0, int argc, char **argv)
{
	if (argc < 2) {
		return print_usage(ctx, argv0, argc, argv);
	}

	for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		if (!strcmp(argv[1], commands[i].name)) {
			return commands[i].func(ctx, argv0, argc - 1, &argv[1]);
		}
	}

	fprintf(stderr, "Invalid command: '%s image %s'\n\n", argv0, argv[1]);

	return print_usage(ctx, argv0, argc, argv);
}
