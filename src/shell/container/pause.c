#include <stdio.h>
#include <string.h>

#include <ocre/ocre.h>

#include "../command.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s container pause CONTAINER\n", argv0);
	fprintf(stderr, "\nPauses a container in the OCRE context.\n");
	return -1;
}

int cmd_container_pause(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	if (argc == 2) {

		struct ocre_container *container = ocre_context_get_container_by_id(ctx, argv[1]);
		if (!container) {
			fprintf(stderr, "Failed to get container '%s'\n", argv[1]);
			return -1;
		}

		if (ocre_container_get_status(container) != OCRE_CONTAINER_STATUS_RUNNING) {
			fprintf(stderr, "Container '%s' is not running\n", argv[1]);
			return -1;
		}

		return ocre_container_pause(container);
	} else {
		fprintf(stderr, "'%s container pause' requires exactly one argument\n\n", argv0);
		return usage(argv0);
	}

	return 0;
}
