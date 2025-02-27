/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_TIMER_H
#define OCRE_TIMER_H

#include <stdbool.h>
#include "wasm_export.h"

/**
 * @typedef ocre_timer_t
 * @brief Timer identifier type
 */
typedef int ocre_timer_t;

void ocre_timer_init(void);

void ocre_timer_cleanup_container(wasm_module_inst_t module_inst);

void ocre_timer_set_module_inst(wasm_module_inst_t module_inst);

/**
 * @brief Creates a timer with the specified ID
 *
 * @param id Timer identifier (must be between 1 and MAX_TIMERS)
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid timer ID
 * @retval EEXIST Timer ID already in use
 */
int ocre_timer_create(wasm_exec_env_t exec_env, int id);

/**
 * @brief Deletes a timer
 *
 *
 * @param id Timer identifier
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid timer ID or timer not found
 */
int ocre_timer_delete(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Starts a timer
 *
 * @param id Timer identifier
 * @param interval Timer interval in milliseconds
 * @param is_periodic True for periodic timer, false for one-shot
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid timer ID or timer not found
 */
int ocre_timer_start(wasm_exec_env_t exec_env, ocre_timer_t id, int interval, int is_periodic);

/**
 * @brief Stops a timer
 *
 * @param id Timer identifier
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid timer ID or timer not found
 */
int ocre_timer_stop(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Gets the remaining time for a timer
 *
 * @param id Timer identifier
 * @return Remaining time in milliseconds, or -1 on error with errno set
 * @retval EINVAL Invalid timer ID or timer not found
 */
int ocre_timer_get_remaining(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Sets the WASM dispatcher function for timer callbacks
 *
 * @param func WASM function instance to be called when timer expires
 */
void ocre_timer_set_dispatcher(wasm_exec_env_t exec_env);

#endif /* OCRE_TIMER_H */
