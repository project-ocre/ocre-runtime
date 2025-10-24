/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_TIMER_H
#define OCRE_TIMER_H

#include <wasm_export.h>
#include "ocre_core_external.h"

#ifndef OCRE_TIMER_T_DEFINED
#define OCRE_TIMER_T_DEFINED
typedef uint32_t ocre_timer_t;
#endif

/**
 * @brief Initialize the timer system.
 */
void ocre_timer_init(void);

/**
 * @brief Create a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @return 0 on success, negative error code on failure.
 */
int ocre_timer_create(wasm_exec_env_t exec_env, int id);

/**
 * @brief Delete a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @return 0 on success, negative error code on failure.
 */
int ocre_timer_delete(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Start a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @param interval Timer interval in milliseconds.
 * @param is_periodic Whether the timer is periodic.
 * @return 0 on success, negative error code on failure.
 */
int ocre_timer_start(wasm_exec_env_t exec_env, ocre_timer_t id, int interval, int is_periodic);

/**
 * @brief Stop a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @return 0 on success, negative error code on failure.
 */
int ocre_timer_stop(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Get remaining time for a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @return Remaining time in milliseconds, or negative error code on failure.
 */
int ocre_timer_get_remaining(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Cleanup timer resources for a module.
 *
 * @param module_inst WASM module instance.
 */
void ocre_timer_cleanup_container(wasm_module_inst_t module_inst);

#endif /* OCRE_TIMER_H */
