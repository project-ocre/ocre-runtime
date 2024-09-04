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
extern state_machine_t ocre_cs_state_machine; // TODO THis needs to get encapsulated into the
                                              // sm. it's only here so components can operate
                                              // timers. timers need to be encapsulated.

enum CONTAINER_RUNTIME_STATE {
    STATE_INITIALIZED,
    STATE_CREATED,
    STATE_RUNNING,
    STATE_STOPPED,
    STATE_DESTROYED,
    STATE_ERROR,
    STATE_RUNTIME_ERROR
};

enum OCRE_CS_EVENT {
    EVENT_CREATE_CONTAINER,
    EVENT_DESTROY_CONTAINER,
    EVENT_RUN_CONTAINER,
    EVENT_STOP_CONTAINER,
    EVENT_RESTART_CONTAINER,
    //  errors
    EVENT_RUNTIME_ERROR,
    EVENT_ERROR
};
enum OCRE_CS_ERROR {
    ERROR_FILE_SYSTEM,
    ERROR_WAMR,
    ERROR_UNDEFINED,
};

int _ocre_cs_run();

#endif
