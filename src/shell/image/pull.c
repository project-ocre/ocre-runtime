#include <stdio.h>
#include <string.h>

#include <ocre/ocre.h>

#include "../command.h"

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s image pull URL\n", argv0); // TODO: NAME[:TAG|@DIGEST]
	fprintf(stderr, "\nDownloads an image from a remote repository to the local storage.\n");
	return -1;
}

int cmd_image_pull(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	if (argc < 1) {
		fprintf(stderr, "'%s image pull' requires at least one argument\n\n", argv0);
		return usage(argv0);
	}

	if (argc == 1) {
		printf("Pulling image '%s'...\n", argv[1]);
	} else {
		printf("Pulling image '%s' and saving locally as '%s'...\n", argv[1], argv[2]);
	}

	return 0;
}
