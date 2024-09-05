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
#include "ocre/ocre_container_runtime/ocre_container_runtime.h"

// Define state machine and component
struct ocre_component ocre_cs_component;
state_machine_t ocre_cs_state_machine;

#define OCRE_CS_DEBUG_ON 0

struct ocre_container_t containers[MAX_CONTAINERS];
int current_container_id = -1;
int download_count = 0;

/* State event handlers */
static void initialized_run(void *o) {
    struct ocre_message *msg = SM_GET_EVENT(o);
    struct ocre_cs_ctx *ctx = SM_GET_CUSTOM_CTX(o);
  
    switch (msg->event) {
        case EVENT_CREATE_CONTAINER:
            LOG_INF("EVENT_CREATE_CONTAINER");
            current_container_id++;
            ctx->current_container = &containers[current_container_id];
        
            if (download_count <= current_container_id) {
                break;
            }
        
            ocre_container_runtime_create_container(SM_GET_CUSTOM_CTX(o), current_container_id);
            ocre_container_runtime_run_container(SM_GET_CUSTOM_CTX(o), current_container_id);
        
            // here is initialized container number [current_container_id]
            sm_transition(&ocre_cs_state_machine, STATE_CREATED);
            break;
        
        case EVENT_RUNTIME_ERROR:
            LOG_INF("EVENT_ERROR");
            sm_transition(&ocre_cs_state_machine, STATE_RUNTIME_ERROR);
            break;
        
        default:
            LOG_INF("EVENT: %d", msg->event);
            break;
    }
  
    SM_MARK_EVENT_HANDLED(o);
}

static void created_run(void *o) {
    SM_RETURN_IF_EVENT_HANDLED(o);
    struct ocre_message *msg = SM_GET_EVENT(o);
    struct ocre_cs_ctx *ctx = SM_GET_CUSTOM_CTX(o);
  
    switch (msg->event) {
        case EVENT_RUN_CONTAINER:
            LOG_INF("EVENT_RUN_CONTAINER");
            sm_transition(&ocre_cs_state_machine, STATE_RUNNING);
            break;
        
        case EVENT_DESTROY_CONTAINER:
            LOG_INF("EVENT_DESTROY_CONTAINER");
            ctx->current_container = &containers[current_container_id];
            ocre_container_runtime_destroy_container(SM_GET_CUSTOM_CTX(o), current_container_id);
            sm_transition(&ocre_cs_state_machine, STATE_DESTROYED);
            break;
        
        case EVENT_ERROR:
            LOG_INF("EVENT_ERROR");
            sm_transition(&ocre_cs_state_machine, STATE_ERROR);
            break;
        
        default:
            LOG_INF("EVENT: %d", msg->event);
            break;
    }
  
    SM_MARK_EVENT_HANDLED(o);
}

static void running_run(void *o) {
    SM_RETURN_IF_EVENT_HANDLED(o);
    struct ocre_message *msg = SM_GET_EVENT(o);
    struct ocre_cs_ctx *ctx = SM_GET_CUSTOM_CTX(o);
  
    if (containers[current_container_id].container_runtime_status == CONTAINER_STATUS_UNRESPONSIVE) {
        // if the container is unresponsive
        ocre_container_runtime_restart_container(&ctx, current_container_id);
        ocre_healthcheck_reinit(&containers[current_container_id].ocre_container_data.WDT);
    }
  
    switch (msg->event) {
        case EVENT_STOP_CONTAINER:
            LOG_INF("EVENT_STOP_CONTAINER");
            ocre_container_runtime_stop_container(SM_GET_CUSTOM_CTX(o), current_container_id);
            sm_transition(&ocre_cs_state_machine, STATE_STOPPED);
            break;
        
        case EVENT_RESTART_CONTAINER:
            LOG_INF("EVENT_RESTART_CONTAINER");
            ocre_container_runtime_restart_container(SM_GET_CUSTOM_CTX(o), current_container_id);
            sm_transition(&ocre_cs_state_machine, STATE_RUNNING);
            break;
        
        case EVENT_ERROR:
            LOG_INF("EVENT_ERROR");
            sm_transition(&ocre_cs_state_machine, STATE_ERROR);
            break;
        default:
            LOG_INF("EVENT: %d", msg->event);
            break;
    }
  
    SM_MARK_EVENT_HANDLED(o);
}

static void stopped_run(void *o) {
    SM_RETURN_IF_EVENT_HANDLED(o);
    struct ocre_cs_ctx *ctx = SM_GET_CUSTOM_CTX(o);
    struct ocre_message *msg = SM_GET_EVENT(o);
  
    switch (msg->event) {
        case EVENT_DESTROY_CONTAINER:
            LOG_INF("EVENT_DESTROY_CONTAINER");
            ctx->current_container = &containers[current_container_id];
            ocre_container_runtime_destroy_container(SM_GET_CUSTOM_CTX(o), current_container_id);
            sm_transition(&ocre_cs_state_machine, STATE_DESTROYED);
            break;
        
        case EVENT_ERROR:
            LOG_INF("EVENT_ERROR");
            sm_transition(&ocre_cs_state_machine, STATE_ERROR);
            break;
        
        case EVENT_RUN_CONTAINER: 
            ocre_container_runtime_run_container(SM_GET_CUSTOM_CTX(o), current_container_id);
            break;
        
        default:
            LOG_INF("EVENT: %d", msg->event);
            break;
    }

    SM_MARK_EVENT_HANDLED(o);
}

static void destroyed_run(void *o) {
    struct ocre_cs_ctx *ctx = SM_GET_CUSTOM_CTX(o);
    current_container_id--;
    ctx->current_container = &containers[current_container_id];
}

static void error_run(void *o) {
}

static void runtime_error_run(void *o) {
}

static const struct smf_state hsm[] = {
        [STATE_INITIALIZED] = SMF_CREATE_STATE(NULL, initialized_run, NULL, NULL, NULL),
        [STATE_CREATED] = SMF_CREATE_STATE(NULL, created_run, NULL, &hsm[STATE_INITIALIZED], NULL),
        [STATE_RUNNING] = SMF_CREATE_STATE(NULL, running_run, NULL, &hsm[STATE_INITIALIZED], NULL),
        [STATE_STOPPED] = SMF_CREATE_STATE(NULL, stopped_run, NULL, &hsm[STATE_INITIALIZED], NULL),
        [STATE_ERROR] = SMF_CREATE_STATE(NULL, error_run, NULL, NULL, &hsm[STATE_INITIALIZED]),
        [STATE_DESTROYED] = SMF_CREATE_STATE(NULL, destroyed_run, NULL, &hsm[STATE_INITIALIZED], NULL),
        [STATE_RUNTIME_ERROR] = SMF_CREATE_STATE(NULL, runtime_error_run, NULL, NULL, NULL)};

// Entry point for running the state machine
int _ocre_cs_run() {
    ocre_cs_ctx ctx;

    ocre_component_init(&ocre_cs_component);

    sm_init(&ocre_cs_state_machine, &ocre_cs_component.msgq, &ocre_cs_component.msg, &ctx, hsm);

    return sm_run(&ocre_cs_state_machine, STATE_INITIALIZED);
}
