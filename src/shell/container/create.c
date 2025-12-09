#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ocre/ocre.h>

#include "../command.h"

static int usage(const char *argv0, char *cmd)
{
	fprintf(stderr, "Usage: %s container %s [options] IMAGE [ARG...]\n", argv0, cmd);
	if (!strcmp(cmd, "create")) {
		fprintf(stderr, "\nCreates a container in the Ocre context.\n");
	} else if (!strcmp(cmd, "run")) {
		fprintf(stderr, "\nCreates and starts a container in the Ocre context.\n");
	}
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "  -d                       Creates a detached container\n");
	fprintf(stderr, "  -n CONTAINER_ID          Specifies a container ID\n");
	fprintf(stderr, "  -r RUNTIME               Specifies the runtime to use\n");
	fprintf(stderr, "  -v VOLUME:MOUNTPOINT     Adds a volume to be mounted into the container\n");
	fprintf(stderr, "  -v /ABSPATH:MOUNTPOINT   Adds a directory to be mounted into the container\n");
	fprintf(stderr, "  -k CAPABILITY            Adds a capability to the container\n");
	fprintf(stderr, "  -e VAR=VALUE             Sets an environment variable in the container\n");
	fprintf(stderr, "\nOptions '-v' and '-e' and '-k' can be supplied multiple times.\n");

	return -1;
}

int cmd_container_create_run(struct ocre_context *ctx, char *argv0, int argc, char **argv)
{
	int ret = -1;

	if (argc < 2) {
		fprintf(stderr, "'%s container %s' requires arguments\n\n", argv0, argv[0]);
		return usage(argv0, argv[0]);
	}

	bool detached = false;
	char *runtime = NULL;
	char *container_id = NULL;
	const char **capabilities = NULL;
	const char **environment = NULL;
	const char **mounts = NULL;

	size_t capabilities_count = 0;
	size_t environment_count = 0;
	size_t mounts_count = 0;

	opterr = 0;

	int opt;
	while ((opt = getopt(argc, argv, "de:k:n:r:v:")) != -1) {
		switch (opt) {
			case 'd': {
				if (detached) {
					fprintf(stderr, "Detached mode can be set only once\n\n");
					usage(argv0, argv[0]);
					goto cleanup;
				}

				detached = true;
				continue;
			}
			case 'n': {
				if (container_id) {
					fprintf(stderr, "Container ID can be set only once\n\n");
					usage(argv0, argv[0]);
					goto cleanup;
				}

				container_id = optarg;
				continue;
			}
			case 'r': {
				if (runtime) {
					fprintf(stderr, "Runtime can be set only once\n\n");
					usage(argv0, argv[0]);
					goto cleanup;
				}

				runtime = optarg;
				continue;
			}
			case 'v': {

				/* Check mounts parameters of format <source>:<destination>
				 * Destination should be absolute path
				 * We do not like anything to be mounted at '/'
				 * We handle '/' with the 'filesystem' capability.
				 */

				if (optarg[0] != '/') {
					fprintf(stderr, "Invalid mount format: '%s': source must be absolute path", optarg);
					return NULL;
				}

				char *dst = strchr(optarg, ':');
				if (!dst) {
					fprintf(stderr, "Invalid mount format: '%s': must be <source>:<destination>", optarg);
					return NULL;
				}

				dst++;

				if (dst[0] != '/') {
					fprintf(stderr, "Invalid mount format: '%s': destination must be absolute path",
						optarg);
					return NULL;
				}

				if (dst[1] == '\0') {
					fprintf(stderr, "Invalid mount format: '%s': destination must not be '/'", optarg);
					return NULL;
				}

				mounts = realloc(mounts, sizeof(char *) * (mounts_count + 1));
				mounts[mounts_count++] = optarg;
				continue;
			}
			case 'k': {
				capabilities = realloc(capabilities, sizeof(char *) * (capabilities_count + 1));
				capabilities[capabilities_count++] = optarg;
				continue;
			}
			case 'e': {
				environment = realloc(environment, sizeof(char *) * (environment_count + 1));
				environment[environment_count++] = optarg;
				continue;
			}
			case '?': {
				fprintf(stderr, "Invalid option '-%c'\n", optopt);
				goto cleanup;
				break;
			}
		}
	}

	environment = realloc(environment, sizeof(char *) * (environment_count + 1));
	environment[environment_count++] = NULL;

	capabilities = realloc(capabilities, sizeof(char *) * (capabilities_count + 1));
	capabilities[capabilities_count++] = NULL;

	mounts = realloc(mounts, sizeof(char *) * (mounts_count + 1));
	mounts[mounts_count++] = NULL;

	if (optind >= argc) {
		fprintf(stderr, "'%s container %s' requires at least one non option argument\n\n", argv0, argv[0]);
		return usage(argv0, argv[0]);
		goto cleanup;
	}

	const struct ocre_container_args arguments = {
		.argv = (const char **)&argv[optind + 1],
		.capabilities = capabilities,
		.envp = environment,
		.mounts = mounts,
	};

	struct ocre_container *container =
		ocre_context_create_container(ctx, argv[optind], runtime, container_id, detached, &arguments);

	if (!container) {
		fprintf(stderr, "Failed to create container\n");
		goto cleanup;
	}

	if (!strcmp(argv[0], "run")) {
		if (ocre_container_start(container)) {
			fprintf(stderr, "Failed to start container\n");
			goto cleanup;
		}
	}

	if (detached) {
		char *cid = ocre_container_get_id_a(container);
		fprintf(stdout, "%s\n", cid);
		free(cid);
	}

	ret = 0;

cleanup:
	free(mounts);
	free(capabilities);
	free(environment);

	return ret;
}
