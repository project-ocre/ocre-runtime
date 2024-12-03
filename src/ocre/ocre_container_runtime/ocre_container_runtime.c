/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef UNIT_TESTING
#include "../../application/tests/utests_ocre_container_runtime/stubs/components/container_supervisor/cs_sm_impl.h"
#include "../../application/tests/utests_ocre_container_runtime/stubs/components/container_supervisor/cs_sm.h"
#include "../../application/tests/utests_ocre_container_runtime/stubs/wasm/wasm.h"
#include "../../application/tests/utests_ocre_container_runtime/stubs/ocre/fs/fs.h"
#else
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <ocre/ocre.h>
#include "../components/container_supervisor/cs_sm.h"
#include "../components/container_supervisor/cs_sm_impl.h"
// WAMR includes
#include "../api/ocre_api.h"
#endif

#include <stdlib.h>
#include <string.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#include "ocre_container_runtime.h"

ocre_container_runtime_status_t ocre_container_runtime_init(ocre_cs_ctx *ctx, ocre_container_init_arguments_t *args) {
    // Zeroing the context
    CS_runtime_init(ctx, args);

    CS_ctx_init(ctx);
    ctx->current_container_id = 0;
    ctx->download_count = 0;

    k_sem_init(&ctx->initialized, 0, 1);

    start_ocre_cs_thread(ctx);

    // Wait for initialization to complete
    int ret = k_sem_take(&ctx->initialized, K_MSEC(OCRE_CR_INIT_TIMEOUT));
    if (ret == 0) {
        return RUNTIME_STATUS_INITIALIZED;
    }

    LOG_ERR("Failure initializing Ocre container runtime");
    return RUNTIME_STATUS_ERROR;
}

ocre_container_status_t ocre_container_runtime_destroy(void) {
    wasm_runtime_destroy();
    destroy_ocre_cs_thread();

    return RUNTIME_STATUS_DESTROYED;
}

ocre_container_status_t ocre_container_runtime_create_container(ocre_cs_ctx *ctx, ocre_container_data_t *container_data,
                                                                int *container_id, ocre_container_runtime_cb callback) {
    int i;
    // Find available slot for new container
    for (i = 0; i < MAX_CONTAINERS; i++) {
        if ((ctx->containers[i].container_runtime_status == CONTAINER_STATUS_UNKNOWN) ||
            (ctx->containers[i].container_runtime_status == CONTAINER_STATUS_DESTROYED)) {
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
                                                             ocre_container_runtime_cb callback) {
    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    LOG_INF("Request run container in slot:%d", container_id);
    struct ocre_message event = {.event = EVENT_RUN_CONTAINER};
    event.containerId = container_id;
    ocre_component_send(&ocre_cs_component, &event);
}

ocre_container_status_t ocre_container_runtime_get_container_status(ocre_cs_ctx *ctx, int container_id) {
    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }

    return ctx->containers[container_id].container_runtime_status;
}

ocre_container_status_t ocre_container_runtime_stop_container(ocre_cs_ctx *ctx, int container_id,
                                                              ocre_container_runtime_cb callback) {
    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }
    LOG_INF("Request stop container in slot:%d", container_id);
    struct ocre_message event = {.event = EVENT_STOP_CONTAINER};
    event.containerId = container_id;
    ocre_component_send(&ocre_cs_component, &event);
}

ocre_container_status_t ocre_container_runtime_destroy_container(ocre_cs_ctx *ctx, int container_id,
                                                                 ocre_container_runtime_cb callback) {
    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }
    LOG_INF("Request destroy container in slot:%d", container_id);
    struct ocre_message event = {.event = EVENT_DESTROY_CONTAINER};
    event.containerId = container_id;
    ocre_component_send(&ocre_cs_component, &event);
}

ocre_container_status_t ocre_container_runtime_restart_container(ocre_cs_ctx *ctx, int container_id,
                                                                 ocre_container_runtime_cb callback) {
    if (container_id < 0 || container_id >= MAX_CONTAINERS) {
        LOG_ERR("Invalid container ID: %d", container_id);
        return CONTAINER_STATUS_ERROR;
    }
    LOG_INF("Request restart container in slot:%d", container_id);
    struct ocre_message event = {.event = EVENT_RESTART_CONTAINER};
    event.containerId = container_id;
    ocre_component_send(&ocre_cs_component, &event);
}
