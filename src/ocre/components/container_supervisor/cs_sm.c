/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#include "cs_sm.h"
#include "cs_sm_impl.h"
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>

// Define state machine and component
struct ocre_component ocre_cs_component;
state_machine_t ocre_cs_state_machine;

/* State event handlers */
static void runtime_uninitialized_entry(void *o)
{
#if OCRE_CS_DEBUG_ON
    LOG_INF("HELLO runtime_uninitialized_entry");
#endif
    struct ocre_message event = {.event = EVENT_CS_INITIALIZE};
    ocre_component_send(&ocre_cs_component, &event);
}
static void runtime_running_entry(void *o)
{
#if OCRE_CS_DEBUG_ON
    LOG_INF("HELLO runtime_running_entry");
#endif
}

static void runtime_uninitialized_run(void *o)
{
#if OCRE_CS_DEBUG_ON
    LOG_INF("HELLO runtime_uninitialized_run");
#endif
    struct ocre_message *msg = SM_GET_EVENT(o);
    ocre_cs_ctx *ctx = SM_GET_CUSTOM_CTX(o);
    switch (msg->event)
    {
    case EVENT_CS_INITIALIZE:
        sm_transition(&ocre_cs_state_machine, STATE_RUNTIME_RUNNING);
        break;

    default:

        LOG_INF("EVENT:%d", msg->event);
        break;
    }
    SM_MARK_EVENT_HANDLED(o);
}
void callbackFcn(void)
{
    LOG_INF("CALLBACK CALLED");
}
static void runtime_running_run(void *o)
{
#if OCRE_CS_DEBUG_ON
    LOG_INF("HELLO runtime_running_run");
#endif
    struct ocre_message *msg = SM_GET_EVENT(o);
    ocre_cs_ctx *ctx = SM_GET_CUSTOM_CTX(o);
    ocre_container_runtime_cb callback = callbackFcn;
    switch (msg->event)
    {
    case EVENT_CREATE_CONTAINER:
    {
        LOG_INF("EVENT_CREATE_CONTAINER");
        if (CS_create_container(ctx, ctx->current_container_id) == CONTAINER_STATUS_CREATED)
        {
            LOG_INF("Created container in slot:%d", ctx->current_container_id);
        }
        else
        {
            LOG_INF("Failed to create container in slot:%d", ctx->current_container_id);
        }
        break;
    }
    case EVENT_RUN_CONTAINER:
    {
        if (CS_run_container(ctx, ctx->current_container_id) == CONTAINER_STATUS_RUNNING)
        {
            LOG_INF("Started container in slot:%d", ctx->current_container_id);
        }
        else
        {
            LOG_INF("Failed to run container in slot:%d", ctx->current_container_id);
        }
        break;
    }
    case EVENT_STOP_CONTAINER:
    {
        CS_stop_container(ctx, ctx->current_container_id, callback);
        break;
    }
    case EVENT_DESTROY_CONTAINER:
    {
        CS_destroy_container(ctx, ctx->current_container_id, callback);
        break;
    }
    case EVENT_RESTART_CONTAINER:
    {
        CS_restart_container(ctx, ctx->current_container_id, callback);
        break;
    }
    case EVENT_CS_DESTROY:
        sm_transition(&ocre_cs_state_machine, STATE_RUNTIME_UNINITIALIZED);
        break;
    default:
        break;
    }

    SM_MARK_EVENT_HANDLED(o);
}

static void runtime_error_run(void *o)
{
}

static const struct smf_state hsm[] = {
    [STATE_RUNTIME_UNINITIALIZED] = SMF_CREATE_STATE(runtime_uninitialized_entry, runtime_uninitialized_run, NULL, NULL, NULL),
    [STATE_RUNTIME_RUNNING] = SMF_CREATE_STATE(runtime_running_entry, runtime_running_run, NULL, NULL, NULL),
    [STATE_RUNTIME_ERROR] = SMF_CREATE_STATE(NULL, runtime_error_run, NULL, NULL, NULL)};

// Entry point for running the state machine
int _ocre_cs_run(ocre_cs_ctx *ctx)
{
    ocre_component_init(&ocre_cs_component);

    sm_init(&ocre_cs_state_machine, &ocre_cs_component.msgq, &ocre_cs_component.msg, ctx, hsm);

    return sm_run(&ocre_cs_state_machine, STATE_RUNTIME_UNINITIALIZED);
}
