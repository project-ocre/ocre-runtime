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
#include "../../ocre/ocre_container_runtime/ocre_container_runtime.h"

// Define state machine and component
struct ocre_component ocre_cs_component;
state_machine_t ocre_cs_state_machine;


/* State event handlers */
static void runtime_initialized(void *o) {
    ocre_runime_container_ctx containers[MAX_CONTAINERS];
}

static void runtime_running(void *o) {
    struct ocre_message *msg = SM_GET_EVENT(o);
    SM_MARK_EVENT_HANDLED(o);
}

static void runtime_stoped(void *o) {
    struct ocre_message *msg = SM_GET_EVENT(o);
    struct ocre_cs_ctx *ctx = SM_GET_CUSTOM_CTX(o);

    SM_MARK_EVENT_HANDLED(o);
}

static void runtime_error(void *o) {
    LOG_INF("Entered runtime_error");
}

static const struct smf_state hsm[] = {
        [STATE_RUNTIME_INITIALIZED] = SMF_CREATE_STATE(NULL, runtime_initialized, NULL, NULL, NULL),
        [STATE_RUNTIME_RUNNING] = SMF_CREATE_STATE(NULL, runtime_running, NULL, NULL, NULL),
        [STATE_RUNTIME_STOPPED] = SMF_CREATE_STATE(NULL, runtime_stoped, NULL, NULL, NULL),
        [STATE_RUNTIME_ERROR] = SMF_CREATE_STATE(NULL, runtime_error, NULL, NULL, NULL)};

// Entry point for running the state machine
int _ocre_cs_run() {
    ocre_cs_ctx ctx;

    ocre_component_init(&ocre_cs_component);

    sm_init(&ocre_cs_state_machine, &ocre_cs_component.msgq, &ocre_cs_component.msg, &ctx, hsm);

    return sm_run(&ocre_cs_state_machine, STATE_RUNTIME_INITIALIZED);
}