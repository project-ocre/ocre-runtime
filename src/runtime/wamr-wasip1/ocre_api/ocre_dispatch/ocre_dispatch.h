/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_DISPATCH_H
#define OCRE_DISPATCH_H

#include <stdbool.h>
#include <stdint.h>
#include <wasm_export.h>

/**
 * @brief Marks a module instance as reactive: after its "main" export
 * returns, native code may keep calling its "loop" and/or "onRequest"
 * exports on their own schedule instead of tearing the instance down.
 *
 * Only one module instance may be active at a time (matches the existing
 * single-container-at-a-time model used elsewhere in Ocre's native API,
 * e.g. HTTP content ownership). Activating a new instance implicitly
 * replaces whichever one was previously active.
 *
 * @param module_inst The WASM module instance to activate.
 * @param exec_env A dedicated exec_env for this instance, created by the
 *                 caller via wasm_runtime_create_exec_env(). Ownership stays
 *                 with the caller; ocre_dispatch never destroys it.
 */
void ocre_dispatch_activate(wasm_module_inst_t module_inst, wasm_exec_env_t exec_env);

/**
 * @brief Deactivates a module instance if it is the currently active one.
 * Safe to call unconditionally on container teardown.
 *
 * @param module_inst The WASM module instance being torn down.
 */
void ocre_dispatch_deactivate(wasm_module_inst_t module_inst);

/**
 * @brief Whether the currently active module instance exports "loop".
 */
bool ocre_dispatch_has_loop(void);

/**
 * @brief Whether the currently active module instance exports "onRequest".
 */
bool ocre_dispatch_has_on_request(void);

/**
 * @brief Calls the active module instance's "loop" export, if any. No-op
 * (and safe to call at any time, from any thread) if no reactive instance
 * is active or it doesn't export "loop". Serialized against
 * ocre_dispatch_call_on_request() so the two never run concurrently inside
 * the same WASM instance.
 */
void ocre_dispatch_call_loop(void);

/**
 * @brief Calls the active module instance's "onRequest" export, if any,
 * passing requestId as its single i32 argument. Serialized against
 * ocre_dispatch_call_loop().
 *
 * @param request_id Opaque request ID (see ocre_http.h) the handler should
 *                    use with the ocre_http_get_... / ocre_http_respond natives.
 * @return true if onRequest was actually found and invoked, false if no
 *         reactive instance is active or it doesn't export "onRequest".
 */
bool ocre_dispatch_call_on_request(int32_t request_id);

#endif /* OCRE_DISPATCH_H */
