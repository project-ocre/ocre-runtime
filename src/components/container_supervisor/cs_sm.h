/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_IWASM_H
#define OCRE_IWASM_H

#include <zephyr/kernel.h>
#include <zephyr/smf.h>

#include <ocre/component/component.h>
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>
#include <ocre/sm/sm.h>

extern struct ocre_component ocre_cs_component;
extern state_machine_t ocre_cs_state_machine; 

// define the states
enum CONTAINER_RUNTIME_STATE {
    STATE_RUNTIME_INITIALIZED,
    STATE_RUNTIME_RUNNING,
    STATE_RUNTIME_STOPPED,
    STATE_RUNTIME_ERROR,
};
enum OCRE_CS_EVENT {
    EVENT_NO_EVENT,
    EVENT_DESTROY_CONTAINER,
    EVENT_CREATE_CONTAINERS,
    EVENT_STOP_CONTAINER,
    EVENT_RUN_CONTAINERS,
    EVENT_CONTAINER_ERROR,
    EVENT_RUNTIME_ERROR
};
enum OCRE_CS_ERROR {
    ERROR_FILE_SYSTEM,
    ERROR_WAMR,
    ERROR_UNDEFINED,
};

typedef struct ocre_cs_ctx {
    void *atym_from_component;
} ocre_cs_ctx;

int _ocre_cs_run();

#endif
