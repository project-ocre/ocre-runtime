/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include "cs_sm.h"
#include "ocre_core_external.h"

LOG_MODULE_REGISTER(ocre_cs_component, OCRE_LOG_LEVEL);

#define OCRE_CS_THREAD_PRIORITY 0

static core_thread_t ocre_cs_thread;
static int ocre_cs_thread_initialized = 0;

static void ocre_cs_main(void *ctx)
{
	wasm_runtime_init_thread_env();
	LOG_INF("Container Supervisor started.");
	int ret = _ocre_cs_run(ctx);
	LOG_ERR("Container Supervisor exited: %d", ret);
	wasm_runtime_destroy_thread_env();
}

// Function to start the thread
void start_ocre_cs_thread(ocre_cs_ctx *ctx)
{
	if (ocre_cs_thread_initialized) {
		LOG_WRN("Container Supervisor thread is already running.");
		return;
	}
	int ret = core_thread_create(&ocre_cs_thread, ocre_cs_main, ctx, "Ocre Container Supervisor",
				     OCRE_CS_THREAD_STACK_SIZE, OCRE_CS_THREAD_PRIORITY);
	if (ret != 0) {
		LOG_ERR("Failed to create Container Supervisor thread: %d", ret);
		return;
	}
	ocre_cs_thread_initialized = 1;
}

void destroy_ocre_cs_thread(void)
{
	if (!ocre_cs_thread_initialized) {
		LOG_ERR("Container Supervisor thread is not running.\n");
		return;
	}
	core_thread_destroy(&ocre_cs_thread);
	ocre_cs_thread_initialized = 0;
}
