#ifndef OCRE_TIMER_H
#define OCRE_TIMER_H

#include <wasm_export.h>
#include <zephyr/kernel.h>

typedef int ocre_timer_t;

/**
 * @brief Timer callback function type
 * @param timer_id ID of the expired timer
 */
typedef void (*timer_dispatcher_t)(uint32_t timer_id);

/**
 * @brief Creates a new timer instance
 * @param exec_env WASM execution environment
 * @param id Unique timer identifier (1-CONFIG_MAX_TIMERS)
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid ID or timer system not initialized
 * @retval EEXIST Timer ID already in use
 */
int ocre_timer_create(wasm_exec_env_t exec_env, int id);

/**
 * @brief Deletes a timer instance
 * @param exec_env WASM execution environment
 * @param id Timer identifier to delete
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Timer not found or not in use
 */
int ocre_timer_delete(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Starts a timer with specified parameters
 * @param exec_env WASM execution environment
 * @param id Timer identifier to start
 * @param interval Initial expiration time in milliseconds
 * @param is_periodic 1 for periodic timer, 0 for one-shot
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid timer ID or interval <= 0
 */
int ocre_timer_start(wasm_exec_env_t exec_env, ocre_timer_t id, int interval, int is_periodic);

/**
 * @brief Stops a running timer
 * @param exec_env WASM execution environment
 * @param id Timer identifier to stop
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Timer not found or not in use
 */
int ocre_timer_stop(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Gets the remaining time for a timer
 * @param exec_env WASM execution environment
 * @param id Timer identifier
 * @return Remaining time in milliseconds, or -1 on error with errno set
 * @retval EINVAL Invalid timer ID or timer not found
 */
int ocre_timer_get_remaining(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Cleans up all timers associated with a WASM module instance
 * @param module_inst WASM module instance to clean up
 */
void ocre_timer_cleanup_container(wasm_module_inst_t module_inst);

/**
 * @brief Initializes the timer subsystem
 * @note Must be called once before any other timer operations
 */
void ocre_timer_init(void);

void ocre_timer_register_module(wasm_module_inst_t module_inst);
int ocre_timer_set_callback(wasm_exec_env_t exec_env, const char *callback_name);
#endif /* OCRE_TIMER_H */