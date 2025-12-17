#include <stdio.h>
#include <string.h>

#include <ocre/ocre.h>

#include "../command.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s container start CONTAINER\n", argv0);
	fprintf(stderr, "\nStarts a container in the Ocre context.\n");
	return -1;
}

int cmd_container_start(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	if (argc == 2) {
		struct ocre_container *container = ocre_context_get_container_by_id(ctx, argv[1]);
		if (!container) {
			fprintf(stderr, "Failed to get container '%s'\n", argv[1]);
			return -1;
		}

		ocre_container_status_t status = ocre_container_get_status(container);
		if (status != OCRE_CONTAINER_STATUS_CREATED && status != OCRE_CONTAINER_STATUS_STOPPED) {
			fprintf(stderr, "Container '%s' is not ready to run\n", argv[1]);
			return -1;
		}

		int rc = ocre_container_start(container);

		fprintf(stdout, "%s\n", argv[1]);

		return rc;
	} else {
		fprintf(stderr, "'%s container start' requires exactly one argument\n\n", argv0);
		return usage(argv0);
	}

	return 0;
}
