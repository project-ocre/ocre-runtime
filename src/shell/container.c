#include <stdio.h>
#include <string.h>

#include <ocre/ocre.h>

#include "command.h"

#include "container/inspect.h"
#include "container/create.h"
#include "container/kill.h"
#include "container/pause.h"
#include "container/ps.h"
#include "container/rm.h"
#include "container/start.h"
#include "container/stop.h"
#include "container/unpause.h"
#include "container/wait.h"

static int print_usage(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	fprintf(stderr, "Usage: %s container <COMMAND>\n", argv0);

	fprintf(stderr, "\nCommands:\n");
	fprintf(stderr, "  run       Created and starts a new container\n");
	fprintf(stderr, "  create    Create a new container\n");
	fprintf(stderr, "  start     Start a container\n");
	// fprintf(stderr, "  stop      Stop a running or paused container\n");
	fprintf(stderr, "  kill      Kill a running or paused container\n");
	// fprintf(stderr, "  pause     Pause a running container\n");
	// fprintf(stderr, "  unpause   Resume a paused container\n");
	fprintf(stderr, "  wait      Wait for a container to exit\n");
	fprintf(stderr, "  ps        List containers\n");
	fprintf(stderr, "  rm        Remove a stopped container\n");
	return 0;
}

static const struct ocre_command commands[] = {
	{"help", print_usage},
	{"run", cmd_container_create_run},
	{"create", cmd_container_create_run},
	{"start", cmd_container_start},
	// {"stop", cmd_container_stop},
	{"kill", cmd_container_kill},
	// {"pause", cmd_container_pause},
	// {"unpause", cmd_container_unpause},
	{"wait", cmd_container_wait},
	{"ps", cmd_container_ps},
	{"rm", cmd_container_rm},
};

int cmd_container(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	if (argc < 2) {
		return print_usage(ctx, argv0, argc, argv);
	}

	for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		if (!strcmp(argv[1], commands[i].name)) {
			return commands[i].func(ctx, argv0, argc - 1, &argv[1]);
		}
	}

	fprintf(stderr, "Invalid command: '%s container %s'\n\n", argv0, argv[1]);

	return print_usage(ctx, argv0, argc, argv);
}
