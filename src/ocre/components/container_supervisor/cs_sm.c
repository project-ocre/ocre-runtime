/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include "ocre_core_external.h"
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);
#ifdef CONFIG_OCRE_SENSORS
#include "../../ocre_sensors/ocre_sensors.h"
#endif
#include "cs_sm.h"
#include "cs_sm_impl.h"
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>

// Define state machine and component
struct ocre_component ocre_cs_component;
state_machine_t ocre_cs_state_machine;

/* State event handlers */
static void runtime_uninitialized_entry(void *o) {
#if OCRE_CS_DEBUG_ON
    LOG_INF("HELLO runtime_uninitialized_entry");
#endif
    struct ocre_message event = {.event = EVENT_CS_INITIALIZE};
    ocre_component_send(&ocre_cs_component, &event);
}

static void runtime_uninitialized_run(void *o) {
#if OCRE_CS_DEBUG_ON
    LOG_INF("HELLO runtime_uninitialized_run");
#endif
    struct ocre_message *msg = SM_GET_EVENT(o);

    switch (msg->event) {
        case EVENT_CS_INITIALIZE:
            LOG_INF("Transitioning from state STATE_RUNTIME_UNINITIALIZED_RUN to state STATE_RUNTIME_RUNNING");
            sm_transition(&ocre_cs_state_machine, STATE_RUNTIME_RUNNING);
            break;

        default:

            LOG_INF("EVENT:%d", msg->event);
            break;
    }
    SM_MARK_EVENT_HANDLED(o);
}

static void runtime_running_entry(void *o) {
#if OCRE_CS_DEBUG_ON
    LOG_INF("HELLO runtime_running_entry");
#endif
}

static void runtime_running_run(void *o) {
#if OCRE_CS_DEBUG_ON
    LOG_INF("HELLO runtime_running_run");
#endif
    struct ocre_message *msg = SM_GET_EVENT(o);
    ocre_cs_ctx *ctx = SM_GET_CUSTOM_CTX(o);
    ocre_container_runtime_cb callback = NULL;
    switch (msg->event) {
        case EVENT_CREATE_CONTAINER: {
            LOG_INF("EVENT_CREATE_CONTAINER");

            if (msg->containerId < 0 || msg->containerId >= CONFIG_MAX_CONTAINERS) {
                LOG_ERR("Invalid container ID: %d", msg->containerId);
                break;
            }

            if (CS_create_container(&ctx->containers[msg->containerId]) == CONTAINER_STATUS_CREATED) {
                LOG_INF("Created container in slot:%d", msg->containerId);
            } else {
                LOG_ERR("Failed to create container in slot:%d", msg->containerId);
            }
            break;
        }
        case EVENT_RUN_CONTAINER: {
            LOG_INF("EVENT_RUN_CONTAINER");

            if (msg->containerId < 0 || msg->containerId >= CONFIG_MAX_CONTAINERS) {
                LOG_ERR("Invalid container ID: %d", msg->containerId);
                break;
            }

            if (CS_run_container(&ctx->containers[msg->containerId]) == CONTAINER_STATUS_RUNNING) {
                LOG_INF("Started container in slot:%d", msg->containerId);
            } else {
                LOG_ERR("Failed to run container in slot:%d", msg->containerId);
            }
            break;
        }
        case EVENT_STOP_CONTAINER: {
            LOG_INF("EVENT_STOP_CONTAINER");

            if (msg->containerId < 0 || msg->containerId >= CONFIG_MAX_CONTAINERS) {
                LOG_ERR("Invalid container ID: %d", msg->containerId);
                break;
            }

            if (CS_stop_container(&ctx->containers[msg->containerId], callback) == CONTAINER_STATUS_STOPPED) {
                LOG_INF("Stopped container in slot:%d", msg->containerId);
            } else {
                LOG_ERR("Failed to stop container in slot:%d", msg->containerId);
            }
            break;
        }
        case EVENT_DESTROY_CONTAINER: {
            LOG_INF("EVENT_DESTROY_CONTAINER");

            if (msg->containerId < 0 || msg->containerId >= CONFIG_MAX_CONTAINERS) {
                LOG_ERR("Invalid container ID: %d", msg->containerId);
                break;
            }

            if (CS_destroy_container(&ctx->containers[msg->containerId], callback) == CONTAINER_STATUS_DESTROYED) {
                LOG_INF("Destroyed container in slot:%d", msg->containerId);
            } else {
                LOG_ERR("Failed to destroy container in slot:%d", msg->containerId);
            }
            break;
        }
        case EVENT_RESTART_CONTAINER: {
            LOG_INF("EVENT_RESTART_CONTAINER");
            if (msg->containerId < 0 || msg->containerId >= CONFIG_MAX_CONTAINERS) {
                LOG_ERR("Invalid container ID: %d", msg->containerId);
                break;
            }

            if (CS_restart_container(&ctx->containers[msg->containerId], callback) == CONTAINER_STATUS_RUNNING) {
                LOG_INF("Container in slot:%d restarted successfully", msg->containerId);
            } else {
                LOG_ERR("Failed to restart container in slot:%d", msg->containerId);
            }
            break;
        }
        case EVENT_CS_DESTROY:
            LOG_INF("EVENT_CS_DESTROY");
            sm_transition(&ocre_cs_state_machine, STATE_RUNTIME_UNINITIALIZED);
            break;
        default:
            break;
    }

    SM_MARK_EVENT_HANDLED(o);
}

static void runtime_error_run(void *o) {
#if OCRE_CS_DEBUG_ON
    LOG_INF("HELLO runtime_error_run");
#endif
}

static const struct smf_state hsm[] = {
        [STATE_RUNTIME_UNINITIALIZED] =
                SMF_CREATE_STATE(runtime_uninitialized_entry, runtime_uninitialized_run, NULL, NULL, NULL),
        [STATE_RUNTIME_RUNNING] = SMF_CREATE_STATE(runtime_running_entry, runtime_running_run, NULL, NULL, NULL),
        [STATE_RUNTIME_ERROR] = SMF_CREATE_STATE(NULL, runtime_error_run, NULL, NULL, NULL)};

// Entry point for running the state machine
int _ocre_cs_run(ocre_cs_ctx *ctx) {
    ocre_component_init(&ocre_cs_component);
    sm_init(&ocre_cs_state_machine, &ocre_cs_component.msgq, &ocre_cs_component.msg, ctx, hsm);
    return sm_run(&ocre_cs_state_machine, STATE_RUNTIME_UNINITIALIZED);
}
