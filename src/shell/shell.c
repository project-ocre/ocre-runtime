#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <ocre/ocre.h> // for context // maybe for api?

#include "command.h"
#include "image.h"
#include "image/ls.h"
#include "image/pull.h"
#include "container.h"
#include "container/kill.h"
#include "container/pause.h"
#include "container/ps.h"
#include "container/rm.h"
#include "container/start.h"
#include "container/unpause.h"
#include "container/stop.h"
#include "container/wait.h"
#include "container/create.h"

static int print_version(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	fprintf(stdout, "Ocre version 1.0\n");
	return 0;
}

static int print_usage(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [-v] <COMMAND>\n", argv0);

	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "  -v        Verbose mode\n");

	fprintf(stderr, "\nCommands:\n");
	fprintf(stderr, "  help      Display this help message\n");
	fprintf(stderr, "  version   Display version information\n");
	fprintf(stderr, "  image     Image manipulation commands\n");
	fprintf(stderr, "  container Container management commands\n");

	fprintf(stderr, "\nShortcut Commands:\n");
	fprintf(stderr, "  ps        container ps\n");
	fprintf(stderr, "  create    container create\n");
	fprintf(stderr, "  run       container run\n");
	fprintf(stderr, "  start     container start\n");
	// fprintf(stderr, "  stop      container stop\n");
	fprintf(stderr, "  kill      container kill\n");
	// fprintf(stderr, "  pause     container pause\n");
	// fprintf(stderr, "  unpause   container unpause\n");
	fprintf(stderr, "  rm        container rm\n");
	fprintf(stderr, "  images    image ls\n");
	fprintf(stderr, "  pull      image pull\n");
	return -1;
}

static const struct ocre_command commands[] = {
	// general commands
	{"help", print_usage},
	{"version", print_version},
	{"image", cmd_image},
	{"container", cmd_container},
	// container shortcuts
	{"ps", cmd_container_ps},
	{"create", cmd_container_create_run},
	{"run", cmd_container_create_run},
	{"start", cmd_container_start},
	// {"stop", cmd_container_stop},
	{"kill", cmd_container_kill},
	// {"pause", cmd_container_pause},
	// {"unpause", cmd_container_unpause},
	{"rm", cmd_container_rm},
	// image shortcuts
	{"images", cmd_image_ls},
	{"pull", cmd_image_pull},
};

int ocre_shell(struct ocre_context *ctx, int argc, char *argv[])
{
	int opt;
	bool verbose = false;
	while ((opt = getopt(argc, argv, "+v")) != -1) {
		switch (opt) {
			case 'v': {
				if (verbose) {
					fprintf(stderr, "'-v' can be set only once\n\n");
					print_usage(ctx, argv[0], argc, argv);
					return -1;
				}

				verbose = true;
				continue;
			}
			case '?': {
				fprintf(stderr, "Invalid option: '%c'\n", optopt);
				return -1;
			}
		}
	}

	if (argc <= optind) {
		return print_usage(ctx, argv[0], argc, argv);
	}

	if (verbose) {
		fprintf(stderr, "Using context: %p\n", ctx);
	}

	int save_optind = optind;

	for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		if (argv[save_optind] && !strcmp(argv[save_optind], commands[i].name)) {
			optind = 1;
			return commands[i].func(ctx, argv[0], argc - save_optind, &argv[save_optind]);
		}
	}

	fprintf(stderr, "Invalid command: '%s %s'\n\n", argv[0], argv[save_optind]);

	return print_usage(ctx, argv[0], argc, argv);

	return 0;
}
