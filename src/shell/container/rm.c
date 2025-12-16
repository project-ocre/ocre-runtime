#include <stdio.h>
#include <string.h>

#include "../command.h"
#include "ocre/ocre.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s container rm CONTAINER\n", argv0);
	fprintf(stderr, "\nRemoves a stopped container from the Ocre context.\n");
	return -1;
}

int cmd_container_rm(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	if (argc == 2) {
		struct ocre_container *container = ocre_context_get_container_by_id(ctx, argv[1]);
		if (!container) {
			fprintf(stderr, "Failed to get container '%s'\n", argv[1]);
			return -1;
		}

		ocre_container_status_t status = ocre_container_get_status(container);
		if (status != OCRE_CONTAINER_STATUS_STOPPED && status != OCRE_CONTAINER_STATUS_CREATED &&
		    status != OCRE_CONTAINER_STATUS_ERROR) {
			fprintf(stderr, "Container '%s' is in use\n", argv[1]);
			return -1;
		}

		int rc = ocre_context_remove_container(ctx, container);

		fprintf(stdout, "%s\n", argv[1]);

		return rc;
	} else {
		fprintf(stderr, "'%s container rm' requires exactly one argument\n\n", argv0);
		return usage(argv0);
	}

	return 0;
}
