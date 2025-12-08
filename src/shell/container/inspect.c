#include <stdio.h>
#include <string.h>

#include <ocre/ocre.h>

#include "../command.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s container status CONTAINER\n", argv0);
	fprintf(stderr, "\nChecks the status of a container in the OCRE context.\n");
	return -1;
}

int cmd_container_inspect(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	if (argc == 1) {
		printf("Getting status of container '%s'...\n", argv[1]);
	} else {
		fprintf(stderr, "'%s container status' requires exactly one argument\n\n", argv0);
		return usage(argv0);
	}

	return 0;
}
