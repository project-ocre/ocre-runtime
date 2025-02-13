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

#define OCRE_CS_THREAD_STACK_SIZE 4096
#define OCRE_CS_THREAD_PRIORITY   0

K_THREAD_STACK_DEFINE(ocre_cs_stack, OCRE_CS_THREAD_STACK_SIZE);
k_tid_t ocre_cs_tid = NULL;
struct k_thread ocre_cs_thread;

static void ocre_cs_main(void *ctx, void *arg1, void *arg2) {
    LOG_INF("Container Supervisor started.");
    int ret = _ocre_cs_run(ctx);
    LOG_ERR("Container Supervisor exited: %d", ret);
}

// Function to start the thread
void start_ocre_cs_thread(ocre_cs_ctx *ctx) {
    ocre_cs_tid = k_thread_create(&ocre_cs_thread, ocre_cs_stack, OCRE_CS_THREAD_STACK_SIZE, ocre_cs_main, ctx, NULL, NULL,
                    OCRE_CS_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(ocre_cs_tid, "Ocre Container Supervisor");
}

void destroy_ocre_cs_thread(void) {
    k_thread_abort(&ocre_cs_thread);
}