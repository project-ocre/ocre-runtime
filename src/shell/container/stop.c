#include <stdio.h>
#include <string.h>

#include <ocre/ocre.h>

#include "../command.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s container stop CONTAINER\n", argv0);
	fprintf(stderr, "\nStops a container in the OCRE context.\n");
	return -1;
}

int cmd_container_stop(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	if (argc == 1) {
		printf("Stopping container '%s'...\n", argv[1]);
	} else {
		fprintf(stderr, "'%s container stop' requires exactly one argument\n\n", argv0);
		return usage(argv0);
	}

	return 0;
}
