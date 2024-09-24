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

K_THREAD_STACK_DEFINE(ocre_cs_stack, OCRE_CS_THREAD_STACK_SIZE);
static struct k_thread ocre_cs_tid;

static k_thread_entry_t ocre_cs_main(void *ctx, void *arg1, void *arg2) {
    int ret = _ocre_cs_run(ctx);
    LOG_ERR("Exited Container Supervisor: %d", ret);
}

// Function to start the thread
void start_ocre_cs_thread(ocre_cs_ctx *ctx) {
    k_thread_create(&ocre_cs_tid, ocre_cs_stack, OCRE_CS_THREAD_STACK_SIZE, ocre_cs_main, ctx, NULL, NULL,
                    OCRE_CS_THREAD_PRIORITY, 0, K_NO_WAIT);
}

void destroy_ocre_cs_thread(void) {
    k_thread_abort(&ocre_cs_tid);
}