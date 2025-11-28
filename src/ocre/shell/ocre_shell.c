/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include "ocre_shell.h"

static ocre_cs_ctx *ctx_internal;

int cmd_ocre_run(const struct shell *shell, size_t argc, char **argv)
{
	if (!ctx_internal) {
		shell_error(shell, "Internal context not initialized.");
		return -1;
	}

	if (argc != 2) {
		shell_error(shell, "Usage: ocre run <container_id>");
		return -EINVAL;
	}

	char *endptr;
	int container_id = (int)strtol(argv[1], &endptr, 10);

	if (endptr == argv[1]) {
		shell_error(shell, "No digits were found in argument <container_id>.\n");
		return -EINVAL;
	} else if (*endptr != '\0') {
		shell_error(shell, "Invalid character: %c\n", *endptr);
		return -EINVAL;
	}

	int ret = -1;
	for (int i = 0; i < CONFIG_MAX_CONTAINERS; i++) {
		if (container_id == ctx_internal->containers[i].container_ID) {
			ret = ocre_container_runtime_run_container(container_id, NULL);
			break;
		}
	}

	if (ret == CONTAINER_STATUS_RUNNING) {
		shell_info(shell, "Container started. Name: %s, ID: %d",
			   ctx_internal->containers[container_id].ocre_container_data.name, container_id);
		return 0;
	} else {
		shell_error(shell, "Failed to run container: %d", container_id);
		return -EIO;
	}
}

int cmd_ocre_stop(const struct shell *shell, size_t argc, char **argv)
{
	if (!ctx_internal) {
		shell_error(shell, "Internal context not initialized.");
		return -1;
	}

	if (argc != 2) {
		shell_error(shell, "Usage: ocre stop <container_name>");
		return -EINVAL;
	}

	const char *name = argv[1];
	shell_info(shell, "OCRE Shell Request to stop container with name: %s", name);

	int ret = -1;
	int container_id = -1;
	for (int i = 0; i < CONFIG_MAX_CONTAINERS; i++) {
		if (strcmp(ctx_internal->containers[i].ocre_container_data.name, name) == 0) {
			container_id = ctx_internal->containers[i].container_ID;
			ret = ocre_container_runtime_stop_container(container_id, NULL);
		}
	}

	if (ret == CONTAINER_STATUS_STOPPED) {
		shell_info(shell, "Container stopped. Name: %s, ID: %d", name, container_id);
	} else if (ret == -1) {
		shell_error(shell, "Failed to found container: %s", name);
	} else {
		shell_error(shell, "Failed to stop container: %s", name);
	}

	return ret;
}

int cmd_ocre_restart(const struct shell *shell, size_t argc, char **argv)
{
	if (!ctx_internal) {
		shell_error(shell, "Internal context not initialized.");
		return -1;
	}

	if (argc != 2) {
		shell_error(shell, "Usage: ocre restart <container_name>");
		return -EINVAL;
	}

	const char *name = argv[1];
	shell_info(shell, "OCRE Shell Request to restart container with name: %s", name);

	int ret = -1;
	int container_id = -1;
	for (int i = 0; i < CONFIG_MAX_CONTAINERS; i++) {
		if (strcmp(ctx_internal->containers[i].ocre_container_data.name, name) == 0) {
			container_id = ctx_internal->containers[i].container_ID;
			ret = ocre_container_runtime_restart_container(ctx_internal, container_id, NULL);
		}
	}

	if (ret == CONTAINER_STATUS_RUNNING) {
		shell_info(shell, "Container restarted. Name: %s, ID: %d", name, container_id);
	} else if (ret == -1) {
		shell_error(shell, "Failed to found container: %s", name);
	} else {
		shell_error(shell, "Failed to restart container: %s, status: %d", name, ret);
	}

	return ret;
}

int cmd_ocre_ls(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!ctx_internal) {
		shell_error(shell, "Internal context not initialized.");
		return -1;
	}

	for (int i = 0; i < CONFIG_MAX_CONTAINERS; i++) {
		shell_info(shell, "Container ID: %d, name: %s, status: %d", ctx_internal->containers[i].container_ID,
			   ctx_internal->containers[i].ocre_container_data.name,
			   ctx_internal->containers[i].container_runtime_status);
	}

	return 0;
}

void register_ocre_shell(ocre_cs_ctx *ctx)
{
	ctx_internal = ctx;

	SHELL_STATIC_SUBCMD_SET_CREATE(
		ocre_subcmds, SHELL_CMD(run, NULL, "Start a new container: ocre run <container_id>", cmd_ocre_run),
		SHELL_CMD(stop, NULL, "Stop a container: ocre stop <container_name>", cmd_ocre_stop),
		SHELL_CMD(restart, NULL, "Restart a container: ocre restart <container_name>", cmd_ocre_restart),
		SHELL_CMD(ls, NULL, "List running containers and their status", cmd_ocre_ls), SHELL_SUBCMD_SET_END);

#if defined(CONFIG_OCRE_SHELL)
	SHELL_CMD_REGISTER(ocre, &ocre_subcmds, "OCRE agent commands", NULL);
#endif
}
