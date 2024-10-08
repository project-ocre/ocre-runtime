#include <ocre/ocre.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

// WAMR includes
// #include "coap_ext.h"
#include "../api/ocre_api.h"

#include <ocre/ocre.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <stdio.h>

#include <ocre/fs/fs.h>
#include <ocre/sm/sm.h>
#include "ocre_container_runtime.h"

#include "../../../../application/src/atym/components/device_manager/device_manager_sm.h"
#include "../../../../application/src/atym/components/device_manager/device_manager_sm_impl.h"

#include "../components/container_supervisor/cs_sm.h"
#include <ocre/components/container_supervisor/cs_sm_impl.h>

#include "../container_healthcheck/ocre_container_healthcheck.h"

ocre_container_runtime_status_t ocre_container_runtime_init(ocre_cs_ctx *ctx, ocre_container_init_arguments_t *args)
{
    // Zeroing the context
    CS_runtime_init(ctx, args);

    CS_ctx_init(ctx);
    ctx->current_container_id = 0;
    ctx->download_count = 0;

    start_ocre_cs_thread(ctx);

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
    // Find available slot for new container
    for (i = 0; i < MAX_CONTAINERS; i++)
    {
        if ((ctx->containers[i].container_runtime_status == 0) ||
            (ctx->containers[i].container_runtime_status == CONTAINER_STATUS_DESTROYED))
        {
            *container_id = i;
            break;
        }
    }
    LOG_INF("Request create new container in slot:%d", *container_id);

    struct ocre_message event = {.event = EVENT_CREATE_CONTAINER};
    ocre_container_data_t Data;
    event.containerId = *container_id;
    Data = *container_data;
    ctx->containers[*container_id].ocre_container_data = Data;
    ctx->download_count++;

    ocre_component_send(&ocre_cs_component, &event);
}

ocre_container_status_t ocre_container_runtime_run_container(ocre_cs_ctx *ctx, int container_id,
                                                             ocre_container_runtime_cb callback)
{
    if (container_id < 0 || container_id >= MAX_CONTAINERS)
    {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    LOG_INF("Request run container in slot:%d", container_id);
    struct ocre_message event = {.event = EVENT_RUN_CONTAINER};
    event.containerId = container_id;
    ocre_component_send(&ocre_cs_component, &event);
}

ocre_container_status_t ocre_container_runtime_get_container_status(ocre_cs_ctx *ctx, int container_id)
{
    if (container_id < 0 || container_id >= MAX_CONTAINERS)
    {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    return ctx->containers[container_id].container_runtime_status;
}

ocre_container_status_t ocre_container_runtime_stop_container(ocre_cs_ctx *ctx, int container_id,
                                                              ocre_container_runtime_cb callback)
{
    if (container_id < 0 || container_id >= MAX_CONTAINERS)
    {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }
    LOG_INF("Request stop container in slot:%d", container_id);
    struct ocre_message event = {.event = EVENT_STOP_CONTAINER};
    event.containerId = container_id;
    ocre_component_send(&ocre_cs_component, &event);
}

ocre_container_status_t ocre_container_runtime_destroy_container(ocre_cs_ctx *ctx, int container_id,
                                                                 ocre_container_runtime_cb callback)
{
    if (container_id < 0 || container_id >= MAX_CONTAINERS)
    {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }
    LOG_INF("Request destroy container in slot:%d", container_id);
    struct ocre_message event = {.event = EVENT_DESTROY_CONTAINER};
    event.containerId = container_id;
    ocre_component_send(&ocre_cs_component, &event);
}

ocre_container_status_t ocre_container_runtime_restart_container(ocre_cs_ctx *ctx, int container_id,
                                                                 ocre_container_runtime_cb callback)
{
    if (container_id < 0 || container_id >= MAX_CONTAINERS)
    {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }
    LOG_INF("Request restart container in slot:%d", container_id);
    struct ocre_message event = {.event = EVENT_RESTART_CONTAINER};
    event.containerId = container_id;
    ocre_component_send(&ocre_cs_component, &event);
}
