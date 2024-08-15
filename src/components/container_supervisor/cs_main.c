/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <ocre/ocre.h>
#include "cs_sm.h"

LOG_MODULE_REGISTER(ocre_cs_component, OCRE_LOG_LEVEL);

#define OCRE_CS_THREAD_STACK_SIZE 2048
#define OCRE_CS_THREAD_PRIORITY   0

 static void ocre_cs_main(void *, void *, void *) {
     int ret = _ocre_cs_run();
     LOG_ERR("Exited Container Supervisor: %d", ret);
}

K_THREAD_DEFINE(ocre_cs_tid, OCRE_CS_THREAD_STACK_SIZE, ocre_cs_main, NULL, NULL, NULL, OCRE_CS_THREAD_PRIORITY, 0, 0);