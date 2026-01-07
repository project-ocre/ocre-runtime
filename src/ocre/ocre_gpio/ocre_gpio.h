/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_GPIO_H
#define OCRE_GPIO_H

#include <zephyr/device.h>
#include <stdbool.h>
#include "wasm_export.h"

//  Default configuration values if not provided by Kconfig
#ifndef CONFIG_OCRE_GPIO_MAX_PINS
#define CONFIG_OCRE_GPIO_MAX_PINS 32
#endif

#ifndef CONFIG_OCRE_GPIO_MAX_PORTS
#define CONFIG_OCRE_GPIO_MAX_PORTS 8
#endif

#ifndef CONFIG_OCRE_GPIO_PINS_PER_PORT
#define CONFIG_OCRE_GPIO_PINS_PER_PORT 16
#endif

/**
 * GPIO pin state
 */
typedef enum {
    OCRE_GPIO_PIN_RESET = 0,
    OCRE_GPIO_PIN_SET = 1
} ocre_gpio_pin_state_t;

/**
 * GPIO pin direction
 */
typedef enum {
    OCRE_GPIO_DIR_INPUT = 0,
    OCRE_GPIO_DIR_OUTPUT = 1
} ocre_gpio_direction_t;

/**
 * GPIO configuration structure
 */
typedef struct {
    int pin; /**< GPIO pin number (logical) */
    int port_idx;
    ocre_gpio_direction_t direction; /**< Pin direction */
} ocre_gpio_config_t;

/**
 * GPIO callback function type
 */
typedef void (*ocre_gpio_callback_t)(int pin, ocre_gpio_pin_state_t state);

/**
 * Initialize GPIO subsystem.
 *
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_init(void);

/**
 * Configure a GPIO pin.
 *
 * @param config GPIO pin configuration
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_configure(const ocre_gpio_config_t *config);

/**
 * Set GPIO pin state.
 *
 * @param pin GPIO pin identifier
 * @param state Desired pin state
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_pin_set(int pin, ocre_gpio_pin_state_t state);

/**
 * Get GPIO pin state.
 *
 * @param pin GPIO pin identifier
 * @return Pin state or negative error code
 */
ocre_gpio_pin_state_t ocre_gpio_pin_get(int pin);

/**
 * Toggle GPIO pin state.
 *
 * @param pin GPIO pin identifier
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_pin_toggle(int pin);

/**
 * Register callback for GPIO pin state changes.
 *
 * @param pin GPIO pin identifier
 * @param callback Callback function
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_register_callback(int pin);

/**
 * Unregister GPIO pin callback.
 *
 * @param pin GPIO pin identifier
 * @return 0 on success, negative error code on failure
 */
int ocre_gpio_unregister_callback(int pin);

/**
 * Clean up GPIO resources for a WASM container.
 *
 * @param module_inst WASM module instance
 */
void ocre_gpio_cleanup_container(wasm_module_inst_t module_inst);

/**
 * Set the WASM function that will handle GPIO callbacks.
 *
 * @param exec_env WASM execution environment
 */
void ocre_gpio_set_dispatcher(wasm_exec_env_t exec_env);

/**
 * @brief Configure a GPIO pin by alias name
 * 
 * @param name GPIO alias name (e.g., "led0", "sw0")
 * @param direction GPIO direction (OCRE_GPIO_DIR_INPUT or OCRE_GPIO_DIR_OUTPUT)
 * @return int 0 on success, negative error code on failure
 */
int ocre_gpio_configure_by_name(const char *name, ocre_gpio_direction_t direction);

/**
 * @brief Set a GPIO pin state by alias name
 * 
 * @param name GPIO alias name (e.g., "led0", "sw0")
 * @param state Pin state (OCRE_GPIO_PIN_SET or OCRE_GPIO_PIN_RESET)
 * @return int 0 on success, negative error code on failure
 */
int ocre_gpio_set_by_name(const char *name, ocre_gpio_pin_state_t state);

/**
 * @brief Toggle a GPIO pin by alias name
 * 
 * @param name GPIO alias name (e.g., "led0", "sw0")
 * @return int 0 on success, negative error code on failure
 */
int ocre_gpio_toggle_by_name(const char *name);

/**
 * @brief Get a GPIO pin state by alias name
 * 
 * @param name GPIO alias name (e.g., "led0", "sw0")
 * @return ocre_gpio_pin_state_t Pin state or negative error code on failure
 */
ocre_gpio_pin_state_t ocre_gpio_get_by_name(const char *name);

// WASM-exposed GPIO functions
int ocre_gpio_wasm_init(wasm_exec_env_t exec_env);
int ocre_gpio_wasm_configure(wasm_exec_env_t exec_env, int port, int P_pin, int direction);
int ocre_gpio_wasm_set(wasm_exec_env_t exec_env, int port, int P_pin, int state);
int ocre_gpio_wasm_get(wasm_exec_env_t exec_env, int port, int P_pin);
int ocre_gpio_wasm_toggle(wasm_exec_env_t exec_env, int port, int P_pin);
int ocre_gpio_wasm_register_callback(wasm_exec_env_t exec_env, int port, int P_pin);
int ocre_gpio_wasm_unregister_callback(wasm_exec_env_t exec_env, int port, int P_pin);

int ocre_gpio_wasm_configure_by_name(wasm_exec_env_t exec_env, const char *name, int direction);
int ocre_gpio_wasm_set_by_name(wasm_exec_env_t exec_env, const char *name, int state);
int ocre_gpio_wasm_get_by_name(wasm_exec_env_t exec_env, const char *name);
int ocre_gpio_wasm_toggle_by_name(wasm_exec_env_t exec_env, const char *name);
int ocre_gpio_wasm_register_callback_by_name(wasm_exec_env_t exec_env, const char *name);
int ocre_gpio_wasm_unregister_callback_by_name(wasm_exec_env_t exec_env, const char *name);

#endif /* OCRE_GPIO_H */