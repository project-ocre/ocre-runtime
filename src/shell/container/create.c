/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ocre/ocre.h>

#include "../command.h"

extern char *optarg;
extern int optind, opterr, optopt;

static int usage(const char *argv0, const char *cmd)
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

int cmd_container_create_run(struct ocre_context *ctx, const char *argv0, int argc, char **argv)
{
	int ret = -1;

	if (argc < 2) {
		fprintf(stderr, "'%s container %s' requires arguments\n\n", argv0, argv[0]);
		return usage(argv0, argv[0]);
	}

	bool detached = false;
	const char *runtime = NULL;
	const char *container_id = NULL;
	const char **capabilities = NULL;
	const char **environment = NULL;
	const char **mounts = NULL;

	size_t capabilities_count = 0;
	size_t environment_count = 0;
	size_t mounts_count = 0;

	int opt;
	while ((opt = getopt(argc, argv, "+de:k:n:r:v:")) != -1) {
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

				/* Check if the provided container ID is valid */

				if (optarg && !ocre_is_valid_id(optarg)) {
					fprintf(stderr,
						"Invalid characters in container ID '%s'. Valid are [a-z0-9_-.] "
						"(lowercase alphanumeric) and cannot start with '.'\n",
						optarg);
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
					fprintf(stderr, "Invalid mount format: '%s': source must be absolute path\n",
						optarg);
					goto cleanup;
				}

				char *dst = strchr(optarg, ':');
				if (!dst) {
					fprintf(stderr, "Invalid mount format: '%s': must be <source>:<destination>\n",
						optarg);
					goto cleanup;
				}

				dst++;

				if (dst[0] != '/') {
					fprintf(stderr,
						"Invalid mount format: '%s': destination must be absolute path\n",
						optarg);
					goto cleanup;
				}

				if (dst[1] == '\0') {
					fprintf(stderr, "Invalid mount format: '%s': destination must not be '/'\n",
						optarg);
					goto cleanup;
				}

				const char **new_mounts = realloc(mounts, sizeof(char *) * (mounts_count + 1));
				if (!new_mounts) {
					goto cleanup;
				}

				mounts = new_mounts;

				mounts[mounts_count++] = optarg;
				continue;
			}
			case 'k': {
				const char **new_capabilities =
					realloc(capabilities, sizeof(char *) * (capabilities_count + 1));
				if (!new_capabilities) {
					goto cleanup;
				}

				capabilities = new_capabilities;

				capabilities[capabilities_count++] = optarg;
				continue;
			}
			case 'e': {
				const char **new_environment =
					realloc(environment, sizeof(char *) * (environment_count + 1));
				if (!new_environment) {
					goto cleanup;
				}

				environment = new_environment;

				environment[environment_count++] = optarg;
				continue;
			}
			case '?': {
				fprintf(stderr, "Invalid option '-%c'\n", optopt);
				goto cleanup;
			}
			default: {
				fprintf(stderr, "Invalid option '-%c'\n", optopt);
				goto cleanup;
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
		usage(argv0, argv[0]);
		goto cleanup;
	}

	/* Check if the provided image ID is valid */

	if (argv[optind] && !ocre_is_valid_id(argv[optind])) {
		fprintf(stderr,
			"Invalid characters in image ID '%s'. Valid are [a-z0-9_-.] (lowercase alphanumeric) and "
			"cannot start with '.'",
			argv[optind]);
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
		const char *cid = ocre_container_get_id(container);

		fprintf(stdout, "%s\n", cid);
	}

	ret = 0;

cleanup:
	free(mounts);
	free(capabilities);
	free(environment);

	return ret;
}
