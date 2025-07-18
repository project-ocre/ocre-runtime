/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_IWASM_H
#define OCRE_IWASM_H

#include "ocre_core_external.h"

#include <ocre/component/component.h>
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>
#include <ocre/sm/sm.h>

#define OCRE_CS_DEBUG_ON 0

extern struct ocre_component ocre_cs_component;
extern state_machine_t ocre_cs_state_machine; // TODO This needs to get encapsulated into the
                                              // sm. it's only here so components can operate
                                              // timers. timers need to be encapsulated.

enum CONTAINER_RUNTIME_STATE {
    STATE_RUNTIME_UNINITIALIZED,
    STATE_RUNTIME_RUNNING,
    STATE_RUNTIME_ERROR
};

enum OCRE_CS_EVENT {
    // CS Events
    EVENT_CS_INITIALIZE,
    EVENT_CS_DESTROY,
    EVENT_CS_ERROR,

    // Container related events
    EVENT_CREATE_CONTAINER,
    EVENT_RUN_CONTAINER,
    EVENT_STOP_CONTAINER,
    EVENT_DESTROY_CONTAINER,
    EVENT_RESTART_CONTAINER,
    EVENT_ERROR
};
void start_ocre_cs_thread(ocre_cs_ctx *ctx);

void destroy_ocre_cs_thread(void);

int _ocre_cs_run();

#endif
