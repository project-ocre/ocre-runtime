#ifndef OCRE_TIMER_H
#define OCRE_TIMER_H

#include <stdbool.h>
#include "wasm_export.h"

/**
 * @brief Maximum number of timers that can be active simultaneously
 */
#define MAX_TIMERS 5

/**
 * @typedef ocre_timer_t
 * @brief Timer identifier type
 */
typedef int ocre_timer_t;

/**
 * @brief Creates a timer with the specified ID
 *
 * @param id Timer identifier (must be between 1 and MAX_TIMERS)
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid timer ID
 * @retval EEXIST Timer ID already in use
 */
int ocre_timer_create(ocre_timer_t id);

/**
 * @brief Deletes a timer
 *
 *
 * @param id Timer identifier
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid timer ID or timer not found
 */
int ocre_timer_delete(ocre_timer_t id);

/**
 * @brief Starts a timer
 *
 * @param id Timer identifier
 * @param interval Timer interval in milliseconds
 * @param is_periodic True for periodic timer, false for one-shot
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid timer ID or timer not found
 */
int ocre_timer_start(ocre_timer_t id, int interval, int is_periodic);

/**
 * @brief Stops a timer
 *
 * @param id Timer identifier
 * @return 0 on success, -1 on error with errno set
 * @retval EINVAL Invalid timer ID or timer not found
 */
int ocre_timer_stop(ocre_timer_t id);

/**
 * @brief Gets the remaining time for a timer
 *
 * @param id Timer identifier
 * @return Remaining time in milliseconds, or -1 on error with errno set
 * @retval EINVAL Invalid timer ID or timer not found
 */
int ocre_timer_get_remaining(ocre_timer_t id);

/**
 * @brief Sets the WASM dispatcher function for timer callbacks
 *
 * @param func WASM function instance to be called when timer expires
 */
void ocre_timer_set_dispatcher(wasm_function_inst_t func);

#endif /* OCRE_TIMER_H */