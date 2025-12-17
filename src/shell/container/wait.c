#include <stdio.h>
#include <string.h>

#include <ocre/ocre.h>

#include "../command.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s container wait CONTAINER\n", argv0);
	fprintf(stderr, "\nWaits for a container to exit.\n");
	return -1;
}

int cmd_container_wait(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	if (argc == 2) {
		struct ocre_container *container = ocre_context_get_container_by_id(ctx, argv[1]);
		if (!container) {
			fprintf(stderr, "Failed to get container '%s'\n", argv[1]);
			return -1;
		}

		ocre_container_status_t status = ocre_container_get_status(container);
		if (status == OCRE_CONTAINER_STATUS_UNKNOWN || status == OCRE_CONTAINER_STATUS_CREATED) {
			fprintf(stderr, "Container '%s' has not started\n", argv[1]);
			return -1;
		}

		int return_code;
		int rc = ocre_container_wait(container, &return_code);
		if (rc) {
			fprintf(stderr, "Failed to wait for container '%s'\n", argv[1]);
			return -1;
		}

		fprintf(stdout, "%d\n", return_code);

		return rc;
	} else {
		fprintf(stderr, "'%s container wait' requires exactly one argument\n\n", argv0);
		return usage(argv0);
	}

	return 0;
}
