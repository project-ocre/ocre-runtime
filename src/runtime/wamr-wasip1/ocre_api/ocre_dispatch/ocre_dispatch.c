/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_dispatch.h"

#include <pthread.h>
#include <unistd.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ocre_dispatch, CONFIG_OCRE_LOG_LEVEL);

#ifndef CONFIG_OCRE_LOOP_INTERVAL_MS
#define CONFIG_OCRE_LOOP_INTERVAL_MS 100
#endif

/* Serializes all calls into the active instance: only one thread may ever
 * execute code inside a given WASM module instance at a time, and loop()
 * (native-timer-driven) and onRequest() (HTTP-thread-driven) are called from
 * different threads. */
static pthread_mutex_t call_lock = PTHREAD_MUTEX_INITIALIZER;
static wasm_module_inst_t active_module_inst;
static wasm_exec_env_t active_exec_env;

static pthread_mutex_t scheduler_lock = PTHREAD_MUTEX_INITIALIZER;
static bool scheduler_started;

static void *loop_scheduler_thread(void *arg)
{
	(void)arg;

	while (1) {
		usleep(CONFIG_OCRE_LOOP_INTERVAL_MS * 1000);
		ocre_dispatch_call_loop();
	}

	return NULL;
}

static void ensure_scheduler_started(void)
{
	pthread_mutex_lock(&scheduler_lock);

	if (!scheduler_started) {
		pthread_t thread;
		int rc = pthread_create(&thread, NULL, loop_scheduler_thread, NULL);

		if (rc) {
			LOG_ERR("Failed to start loop() scheduler thread: rc=%d", rc);
		} else {
			pthread_detach(thread);
			scheduler_started = true;
		}
	}

	pthread_mutex_unlock(&scheduler_lock);
}

void ocre_dispatch_activate(wasm_module_inst_t module_inst, wasm_exec_env_t exec_env)
{
	pthread_mutex_lock(&call_lock);
	active_module_inst = module_inst;
	active_exec_env = exec_env;
	pthread_mutex_unlock(&call_lock);

	ensure_scheduler_started();
}

void ocre_dispatch_deactivate(wasm_module_inst_t module_inst)
{
	pthread_mutex_lock(&call_lock);
	if (active_module_inst == module_inst) {
		active_module_inst = NULL;
		active_exec_env = NULL;
	}
	pthread_mutex_unlock(&call_lock);
}

static wasm_function_inst_t lookup_locked(const char *name)
{
	if (!active_module_inst) {
		return NULL;
	}
	return wasm_runtime_lookup_function(active_module_inst, name);
}

bool ocre_dispatch_has_loop(void)
{
	pthread_mutex_lock(&call_lock);
	bool has = lookup_locked("loop") != NULL;
	pthread_mutex_unlock(&call_lock);
	return has;
}

bool ocre_dispatch_has_on_request(void)
{
	pthread_mutex_lock(&call_lock);
	bool has = lookup_locked("onRequest") != NULL;
	pthread_mutex_unlock(&call_lock);
	return has;
}

void ocre_dispatch_call_loop(void)
{
	pthread_mutex_lock(&call_lock);

	wasm_function_inst_t func = lookup_locked("loop");

	if (func && active_exec_env) {
		if (!wasm_runtime_call_wasm(active_exec_env, func, 0, NULL)) {
			const char *exception = wasm_runtime_get_exception(active_module_inst);

			LOG_WRN("loop() call failed: %s", exception ? exception : "unknown");
			wasm_runtime_clear_exception(active_module_inst);
		}
	}

	pthread_mutex_unlock(&call_lock);
}

bool ocre_dispatch_call_on_request(int32_t request_id)
{
	pthread_mutex_lock(&call_lock);

	wasm_function_inst_t func = lookup_locked("onRequest");
	bool called = false;

	if (func && active_exec_env) {
		uint32_t argv[1];

		argv[0] = (uint32_t)request_id;
		called = true;

		if (!wasm_runtime_call_wasm(active_exec_env, func, 1, argv)) {
			const char *exception = wasm_runtime_get_exception(active_module_inst);

			LOG_WRN("onRequest() call failed: %s", exception ? exception : "unknown");
			wasm_runtime_clear_exception(active_module_inst);
		}
	}

	pthread_mutex_unlock(&call_lock);
	return called;
}
