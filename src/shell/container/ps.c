/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ocre/ocre.h>

#include "../command.h"

static const char *container_statuses[] = {"UNKNOWN", //
					   "CREATED", //
					   "RUNNING", //
					   "PAUSED ", //
					   "EXITED ", //
					   "STOPPED", //
					   "ERROR  "};

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s container ps [CONTAINER]\n", argv0);
	fprintf(stderr, "\nList containers in Ocre context.\n");
	return -1;
}

static void header(void)
{
	printf("ID\tSTATUS\tIMAGE\n");
}

static int list_container(struct ocre_container *container)
{
	const char *id = ocre_container_get_id(container);
	const char *image = ocre_container_get_image(container);
	ocre_container_status_t status = ocre_container_get_status(container);

	if (!id || !image || status == OCRE_CONTAINER_STATUS_UNKNOWN) {
		return -1;
	}

	printf("%s\t%s\t%s\n", id, container_statuses[status], image);

	return 0;
}

static int list_containers(struct ocre_context *ctx)
{
	int ret = -1;
	int num_containers = ocre_context_get_container_count(ctx);
	if (num_containers < 0) {
		fprintf(stderr, "Failed to get number of containers\n");
		return -1;
	}

	if (num_containers == 0) {
		return 0;
	}

	struct ocre_container **containers = malloc(sizeof(struct ocre_container *) * num_containers);
	if (!containers) {
		fprintf(stderr, "Failed to allocate memory for containers\n");
		return -1;
	}

	num_containers = ocre_context_get_containers(ctx, containers, num_containers);
	if (num_containers < 0) {
		fprintf(stderr, "Failed to list containers\n");
		goto finish;
	}

	for (int i = 0; i < num_containers; i++) {
		if (list_container(containers[i])) {
			fprintf(stderr, "Failed to list container %d\n", i);
			goto finish;
		}
	}

	ret = 0;

finish:
	free(containers);
	return ret;
}

int cmd_container_ps(struct ocre_context *ctx, const char *argv0, int argc, char **argv)
{
	switch (argc) {
		case 1: {
			header();
			return list_containers(ctx);
		}
		case 2: {
			struct ocre_container *container = ocre_context_get_container_by_id(ctx, argv[1]);
			if (!container) {
				fprintf(stderr, "Failed to get container '%s'\n", argv[1]);
				return -1;
			}

			header();
			return list_container(container);
		}
		default:
			fprintf(stderr, "'%s container ps' requires at most one argument\n\n", argv0);
			return usage(argv0);
	}
}
