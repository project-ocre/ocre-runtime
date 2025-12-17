/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include "ocre_core_external.h"
#include "../components/container_supervisor/cs_sm.h"
#include "../components/container_supervisor/cs_sm_impl.h"
// WAMR includes
// #include "coap_ext.h"
#include "../api/ocre_api.h"

#include <stdlib.h>
#include <string.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#include "ocre_container_runtime.h"

ocre_container_runtime_status_t ocre_container_runtime_init(ocre_cs_ctx *ctx, ocre_container_init_arguments_t *args)
{
	// Zeroing the context
	if (CS_runtime_init(ctx, args) != RUNTIME_STATUS_INITIALIZED) {
		LOG_ERR("Failed to initialize container runtime");
		return RUNTIME_STATUS_ERROR;
	}

	CS_ctx_init(ctx);
	ctx->download_count = 0;

	start_ocre_cs_thread(ctx);
	core_sleep_ms(1000);
	return RUNTIME_STATUS_INITIALIZED;
}

ocre_container_status_t ocre_container_runtime_destroy(void)
{
	wasm_runtime_destroy();
	destroy_ocre_cs_thread();
	return RUNTIME_STATUS_DESTROYED;
}

ocre_container_status_t ocre_container_runtime_create_container(ocre_cs_ctx *ctx, ocre_container_data_t *container_data,
								int *container_id, ocre_container_runtime_cb callback)
{
	int i;
	uint8_t validity_flag = false;
	// Find available slot for new container
	for (i = 0; i < CONFIG_MAX_CONTAINERS; i++) {
		if ((ctx->containers[i].container_runtime_status == CONTAINER_STATUS_UNKNOWN) ||
		    (ctx->containers[i].container_runtime_status == CONTAINER_STATUS_DESTROYED)) {
			*container_id = i;
			ctx->containers[i].container_ID = i;
			ctx->containers[i].container_runtime_status = CONTAINER_STATUS_RESERVED;
			validity_flag = true;
			break;
		}
	}

	if (validity_flag == false) {
		LOG_ERR("No available slots, unable to create container");
		return CONTAINER_STATUS_ERROR;
	}
	LOG_INF("Request to create new container in slot: %d", *container_id);

	struct ocre_message event = {.event = EVENT_CREATE_CONTAINER};
	ocre_container_data_t Data;
	event.containerId = *container_id;
	Data = *container_data;
	ctx->containers[*container_id].ocre_container_data = Data;
	ctx->containers[*container_id].ocre_runtime_arguments.module_inst = NULL;
	ctx->download_count++;

	ocre_component_send(&ocre_cs_component, &event);
	return CONTAINER_STATUS_CREATED;
}

ocre_container_status_t ocre_container_runtime_run_container(int container_id, ocre_container_runtime_cb callback)
{
	if (container_id < 0 || container_id >= CONFIG_MAX_CONTAINERS) {
		LOG_ERR("Invalid container ID: %d", container_id);
		return CONTAINER_STATUS_ERROR;
	}

	LOG_INF("Request to run container in slot:%d", container_id);
	struct ocre_message event = {.event = EVENT_RUN_CONTAINER};
	event.containerId = container_id;
	ocre_component_send(&ocre_cs_component, &event);
	return CONTAINER_STATUS_RUNNING;
}

ocre_container_status_t ocre_container_runtime_get_container_status(ocre_cs_ctx *ctx, int container_id)
{
	if (container_id < 0 || container_id >= CONFIG_MAX_CONTAINERS) {
		LOG_ERR("Invalid container ID: %d", container_id);
		return CONTAINER_STATUS_ERROR;
	}

	return ctx->containers[container_id].container_runtime_status;
}

ocre_container_status_t ocre_container_runtime_stop_container(int container_id, ocre_container_runtime_cb callback)
{
	if (container_id < 0 || container_id >= CONFIG_MAX_CONTAINERS) {
		LOG_ERR("Invalid container ID: %d", container_id);
		return CONTAINER_STATUS_ERROR;
	}
	LOG_INF("Request to stop container in slot: %d", container_id);
	struct ocre_message event = {.event = EVENT_STOP_CONTAINER};
	event.containerId = container_id;
	ocre_component_send(&ocre_cs_component, &event);
	return CONTAINER_STATUS_STOPPED;
}

ocre_container_status_t ocre_container_runtime_destroy_container(ocre_cs_ctx *ctx, int container_id,
								 ocre_container_runtime_cb callback)
{
	if (container_id < 0 || container_id >= CONFIG_MAX_CONTAINERS) {
		LOG_ERR("Invalid container ID: %d", container_id);
		return CONTAINER_STATUS_ERROR;
	}
	LOG_INF("Request to destroy container in slot: %d", container_id);
	struct ocre_message event = {.event = EVENT_DESTROY_CONTAINER};
	event.containerId = container_id;
	ocre_component_send(&ocre_cs_component, &event);
	return CONTAINER_STATUS_DESTROYED;
}

ocre_container_status_t ocre_container_runtime_restart_container(ocre_cs_ctx *ctx, int container_id,
								 ocre_container_runtime_cb callback)
{
	if (container_id < 0 || container_id >= CONFIG_MAX_CONTAINERS) {
		LOG_ERR("Invalid container ID: %d", container_id);
		return CONTAINER_STATUS_ERROR;
	}
	LOG_INF("Request to restart container in slot: %d", container_id);
	struct ocre_message event = {.event = EVENT_RESTART_CONTAINER};
	event.containerId = container_id;
	ocre_component_send(&ocre_cs_component, &event);
	return ctx->containers[container_id].container_runtime_status;
}
