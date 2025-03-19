/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include "ocre_gpio.h"
#include "wasm_export.h"
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#define GPIO_STACK_SIZE      2048
#define GPIO_THREAD_PRIORITY 5
#define WASM_STACK_SIZE      (8 * 1024)

typedef struct {
    bool in_use;
    uint32_t id;
    const struct device *port;
    gpio_pin_t pin_number;
    ocre_gpio_direction_t direction;
    struct gpio_callback gpio_cb_data;
    ocre_gpio_callback_t user_callback;
} gpio_pin_ocre;

static K_THREAD_STACK_DEFINE(gpio_thread_stack, GPIO_STACK_SIZE);
static struct k_thread gpio_thread;
static k_tid_t gpio_thread_id;

K_MSGQ_DEFINE(gpio_msgq, sizeof(uint32_t) * 2, 10, 4);

// GPIO pin management
static gpio_pin_ocre gpio_pins[CONFIG_OCRE_GPIO_MAX_PINS] = {0};
static wasm_function_inst_t gpio_dispatcher_func = NULL;
static bool gpio_system_initialized = false;
static wasm_module_inst_t current_module_inst = NULL;
static wasm_exec_env_t shared_exec_env = NULL;

// Define GPIO port devices structure - will be populated at runtime
static const struct device *gpio_ports[CONFIG_OCRE_GPIO_MAX_PORTS] = {NULL};

// Board-specific GPIO port device definitions
#if defined(CONFIG_BOARD_B_U585I_IOT02A)
// STM32U5 board configuration
#define GPIO_PORT_A DT_NODELABEL(gpioa)
#define GPIO_PORT_B DT_NODELABEL(gpiob)
#define GPIO_PORT_C DT_NODELABEL(gpioc)
#define GPIO_PORT_D DT_NODELABEL(gpiod)
#define GPIO_PORT_E DT_NODELABEL(gpioe)
#define GPIO_PORT_F DT_NODELABEL(gpiof)
#define GPIO_PORT_G DT_NODELABEL(gpiog)
#define GPIO_PORT_H DT_NODELABEL(gpioh)
static const char *gpio_port_names[CONFIG_OCRE_GPIO_MAX_PORTS] = {"GPIOA", "GPIOB", "GPIOC", "GPIOD",
                                                                  "GPIOE", "GPIOF", "GPIOG", "GPIOH"};
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM)
// ESP32C3 board configuration
#define GPIO_PORT_0 DT_NODELABEL(gpio0)
static const char *gpio_port_names[CONFIG_OCRE_GPIO_MAX_PORTS] = {"GPIO0"};
#else
// Generic fallback configuration
#if DT_NODE_EXISTS(DT_NODELABEL(gpio0))
#define GPIO_PORT_0 DT_NODELABEL(gpio0)
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(gpioa))
#define GPIO_PORT_A DT_NODELABEL(gpioa)
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(gpio))
#define GPIO_PORT_G DT_NODELABEL(gpio)
#endif
static const char *gpio_port_names[CONFIG_OCRE_GPIO_MAX_PORTS] = {"GPIO"};
#endif

void ocre_gpio_cleanup_container(wasm_module_inst_t module_inst) {
    if (!gpio_system_initialized || !module_inst) {
        LOG_ERR("GPIO system not properly initialized");
        return;
    }

    if (module_inst != current_module_inst) {
        LOG_WRN("Cleanup requested for non-active module instance");
        return;
    }

    int cleaned_count = 0;
    for (int i = 0; i < CONFIG_OCRE_GPIO_MAX_PINS; i++) {
        if (gpio_pins[i].in_use) {
            if (gpio_pins[i].direction == OCRE_GPIO_DIR_INPUT && gpio_pins[i].user_callback) {
                gpio_pin_interrupt_configure(gpio_pins[i].port, gpio_pins[i].pin_number, GPIO_INT_DISABLE);
                gpio_remove_callback(gpio_pins[i].port, &gpio_pins[i].gpio_cb_data);
            }
            gpio_pins[i].in_use = false;
            gpio_pins[i].id = 0;
            cleaned_count++;
        }
    }

    LOG_INF("Cleaned up %d GPIO pins for container", cleaned_count);
}

static void gpio_thread_fn(void *arg1, void *arg2, void *arg3) {
    uint32_t msg_data[2]; // [0] = pin_id, [1] = state

    while (1) {
        if (k_msgq_get(&gpio_msgq, &msg_data, K_FOREVER) == 0) {
            if (!gpio_dispatcher_func || !current_module_inst || !shared_exec_env) {
                LOG_ERR("GPIO system not properly initialized");
                continue;
            }

            uint32_t pin_id = msg_data[0];
            uint32_t state = msg_data[1];

            LOG_DBG("Processing GPIO pin event: %d, state: %d", pin_id, state);
            uint32_t args[2] = {pin_id, state};

            bool call_success = wasm_runtime_call_wasm(shared_exec_env, gpio_dispatcher_func, 2, args);

            if (!call_success) {
                const char *error = wasm_runtime_get_exception(current_module_inst);
                LOG_ERR("Failed to call WASM function: %s", error ? error : "Unknown error");
            } else {
                LOG_INF("Successfully called WASM function for GPIO pin %d", pin_id);
            }
        }
    }
}

static void gpio_callback_handler(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
    for (int i = 0; i < CONFIG_OCRE_GPIO_MAX_PINS; i++) {
        if (gpio_pins[i].in_use && &gpio_pins[i].gpio_cb_data == cb && gpio_pins[i].port == port) {
            if (pins & BIT(gpio_pins[i].pin_number)) {
                int state = gpio_pin_get(port, gpio_pins[i].pin_number);
                if (state >= 0) {
                    uint32_t msg_data[2] = {gpio_pins[i].id, (uint32_t)state};
                    if (k_msgq_put(&gpio_msgq, &msg_data, K_NO_WAIT) != 0) {
                        LOG_ERR("Failed to queue GPIO callback for ID: %d", gpio_pins[i].id);
                    }
                }
            }
        }
    }
}

void ocre_gpio_set_module_inst(wasm_module_inst_t module_inst) {
    current_module_inst = module_inst;
    if (shared_exec_env) {
        wasm_runtime_destroy_exec_env(shared_exec_env);
    }
    shared_exec_env = wasm_runtime_create_exec_env(module_inst, WASM_STACK_SIZE);
}

void ocre_gpio_set_dispatcher(wasm_exec_env_t exec_env) {
    if (!current_module_inst) {
        LOG_ERR("No active WASM module instance");
        return;
    }

    wasm_function_inst_t func = wasm_runtime_lookup_function(current_module_inst, "gpio_callback");
    if (!func) {
        LOG_ERR("Failed to find 'gpio_callback' in WASM module");
        return;
    }

    gpio_dispatcher_func = func;
    LOG_INF("WASM GPIO dispatcher function set successfully");
}

static gpio_pin_ocre *get_gpio_from_id(int id) {
    if (id < 0 || id >= CONFIG_OCRE_GPIO_MAX_PINS) {
        return NULL;
    }
    return &gpio_pins[id];
}

int ocre_gpio_init(void) {
    if (gpio_system_initialized) {
        LOG_INF("GPIO already initialized");
        return 0;
    }

    // Initialize GPIO ports array based on board
    int port_count = 0;

#if defined(CONFIG_BOARD_B_U585I_IOT02A)
    // STM32U5 board initialization
    if (DT_NODE_EXISTS(GPIO_PORT_A)) {
        gpio_ports[0] = DEVICE_DT_GET(GPIO_PORT_A);
        if (!device_is_ready(gpio_ports[0])) {
            LOG_ERR("GPIOA not ready");
        } else {
            LOG_INF("GPIOA initialized");
            port_count++;
        }
    }

    if (DT_NODE_EXISTS(GPIO_PORT_B)) {
        gpio_ports[1] = DEVICE_DT_GET(GPIO_PORT_B);
        if (!device_is_ready(gpio_ports[1])) {
            LOG_ERR("GPIOB not ready");
        } else {
            LOG_INF("GPIOB initialized");
            port_count++;
        }
    }

    if (DT_NODE_EXISTS(GPIO_PORT_C)) {
        gpio_ports[2] = DEVICE_DT_GET(GPIO_PORT_C);
        if (!device_is_ready(gpio_ports[2])) {
            LOG_ERR("GPIOC not ready");
        } else {
            LOG_INF("GPIOC initialized");
            port_count++;
        }
    }

    if (DT_NODE_EXISTS(GPIO_PORT_D)) {
        gpio_ports[3] = DEVICE_DT_GET(GPIO_PORT_D);
        if (!device_is_ready(gpio_ports[3])) {
            LOG_ERR("GPIOD not ready");
        } else {
            LOG_INF("GPIOD initialized");
            port_count++;
        }
    }

    if (DT_NODE_EXISTS(GPIO_PORT_E)) {
        gpio_ports[4] = DEVICE_DT_GET(GPIO_PORT_E);
        if (!device_is_ready(gpio_ports[4])) {
            LOG_ERR("GPIOE not ready");
        } else {
            LOG_INF("GPIOE initialized");
            port_count++;
        }
    }

    if (DT_NODE_EXISTS(GPIO_PORT_F)) {
        gpio_ports[5] = DEVICE_DT_GET(GPIO_PORT_F);
        if (!device_is_ready(gpio_ports[5])) {
            LOG_ERR("GPIOF not ready");
        } else {
            LOG_INF("GPIOF initialized");
            port_count++;
        }
    }

    if (DT_NODE_EXISTS(GPIO_PORT_G)) {
        gpio_ports[6] = DEVICE_DT_GET(GPIO_PORT_G);
        if (!device_is_ready(gpio_ports[6])) {
            LOG_ERR("GPIOG not ready");
        } else {
            LOG_INF("GPIOG initialized");
            port_count++;
        }
    }

    if (DT_NODE_EXISTS(GPIO_PORT_H)) {
        gpio_ports[7] = DEVICE_DT_GET(GPIO_PORT_H);
        if (!device_is_ready(gpio_ports[7])) {
            LOG_ERR("GPIOH not ready");
        } else {
            LOG_INF("GPIOH initialized");
            port_count++;
        }
    }
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM)
    // ESP32C3 board initialization
    if (DT_NODE_EXISTS(GPIO_PORT_0)) {
        gpio_ports[0] = DEVICE_DT_GET(GPIO_PORT_0);
        if (!device_is_ready(gpio_ports[0])) {
            LOG_ERR("GPIO0 not ready");
        } else {
            LOG_INF("GPIO0 initialized");
            port_count++;
        }
    }
/*                       ADD BOARDS HERE                              */
#else
    // Generic fallback initialization
    // #if defined(GPIO_PORT_0)
    // #if DT_NODE_EXISTS(GPIO_PORT_0)
    //     gpio_ports[0] = DEVICE_DT_GET(GPIO_PORT_0);
    //     LOG_INF("No Board Choosen Generic fallback initialization %s\n", gpio_ports[0]);
    //     port_count = 1;
    // #endif
    // #endif

#if defined(GPIO_PORT_A)
#if DT_NODE_EXISTS(GPIO_PORT_A)
    gpio_ports[0] = DEVICE_DT_GET(GPIO_PORT_A);
    LOG_INF("No Board Choosen Generic fallback initialization %s\n", gpio_ports[0]);
    port_count = 1;
#endif
#endif

#if defined(GPIO_PORT_G)
#if DT_NODE_EXISTS(GPIO_PORT_G)
    gpio_ports[0] = DEVICE_DT_GET(GPIO_PORT_G);
    LOG_INF("No Board Choosen Generic fallback initialization %s\n", gpio_ports[0]);
    port_count = 1;
#endif
#endif
#endif

    if (port_count == 0) {
        LOG_ERR("No GPIO ports were initialized\n");
        return -ENODEV;
    }

    // Start GPIO thread for callbacks
    gpio_thread_id = k_thread_create(&gpio_thread, gpio_thread_stack, K_THREAD_STACK_SIZEOF(gpio_thread_stack),
                                     gpio_thread_fn, NULL, NULL, NULL, GPIO_THREAD_PRIORITY, 0, K_NO_WAIT);
    if (!gpio_thread_id) {
        LOG_ERR("Failed to create GPIO thread");
        return -ENOMEM;
    }

    k_thread_name_set(gpio_thread_id, "gpio_thread");
    LOG_INF("GPIO system initialized with %d ports", port_count);
    gpio_system_initialized = true;
    return 0;
}

int ocre_gpio_configure(const ocre_gpio_config_t *config) {
    if (!gpio_system_initialized) {
        LOG_ERR("GPIO not initialized");
        return -ENODEV;
    }

    if (config == NULL) {
        LOG_ERR("NULL config pointer");
        return -EINVAL;
    }

    if (config->pin < 0 || config->pin >= CONFIG_OCRE_GPIO_MAX_PINS) {
        LOG_ERR("Invalid GPIO pin number: %d", config->pin);
        return -EINVAL;
    }

    // Map logical pin to physical port/pin
    int port_idx = config->pin / CONFIG_OCRE_GPIO_PINS_PER_PORT;
    gpio_pin_t pin_number = config->pin % CONFIG_OCRE_GPIO_PINS_PER_PORT;

    if (port_idx >= CONFIG_OCRE_GPIO_MAX_PORTS || gpio_ports[port_idx] == NULL) {
        LOG_ERR("Invalid GPIO port index: %d", port_idx);
        return -EINVAL;
    }

    // Configure Zephyr GPIO
    gpio_flags_t flags = 0;

    if (config->direction == OCRE_GPIO_DIR_INPUT) {
        flags = GPIO_INPUT;
    } else if (config->direction == OCRE_GPIO_DIR_OUTPUT) {
        flags = GPIO_OUTPUT;
    } else {
        LOG_ERR("Invalid GPIO direction");
        return -EINVAL;
    }

    int ret = gpio_pin_configure(gpio_ports[port_idx], pin_number, flags);
    if (ret != 0) {
        LOG_ERR("Failed to configure GPIO pin: %d", ret);
        return ret;
    }

    // Update our tracking information
    gpio_pins[config->pin].in_use = true;
    gpio_pins[config->pin].id = config->pin;
    gpio_pins[config->pin].port = gpio_ports[port_idx];
    gpio_pins[config->pin].pin_number = pin_number;
    gpio_pins[config->pin].direction = config->direction;
    gpio_pins[config->pin].user_callback = NULL;

    LOG_DBG("Configured GPIO pin %d: port=%s, pin=%d, direction=%d", config->pin, gpio_port_names[port_idx], pin_number,
            config->direction);

    return 0;
}

int ocre_gpio_pin_set(int pin, ocre_gpio_pin_state_t state) {
    gpio_pin_ocre *gpio = get_gpio_from_id(pin);

    if (!gpio_system_initialized) {
        LOG_ERR("GPIO system not initialized when trying to set pin %d", pin);
        return -ENODEV;
    }

    if (!gpio || !gpio->in_use) {
        LOG_ERR("Invalid or unconfigured GPIO pin: %d", pin);
        return -EINVAL;
    }

    if (gpio->direction != OCRE_GPIO_DIR_OUTPUT) {
        LOG_ERR("Cannot set state of input pin: %d", pin);
        return -EINVAL;
    }

    LOG_INF("Setting GPIO pin %d (port=%p, pin=%d) to state %d", pin, gpio->port, gpio->pin_number, state);

    int ret = gpio_pin_set(gpio->port, gpio->pin_number, state);
    if (ret != 0) {
        LOG_ERR("Failed to set GPIO pin %d: %d", pin, ret);
        return ret;
    }

    LOG_INF("Successfully set GPIO pin %d to state %d", pin, state);
    return 0;
}

ocre_gpio_pin_state_t ocre_gpio_pin_get(int pin) {
    gpio_pin_ocre *gpio = get_gpio_from_id(pin);

    if (!gpio_system_initialized) {
        return -ENODEV;
    }

    if (!gpio || !gpio->in_use) {
        LOG_ERR("Invalid or unconfigured GPIO pin: %d", pin);
        return -EINVAL;
    }

    int value = gpio_pin_get(gpio->port, gpio->pin_number);
    if (value < 0) {
        LOG_ERR("Failed to get GPIO pin %d state: %d", pin, value);
        return value;
    }

    return value ? OCRE_GPIO_PIN_SET : OCRE_GPIO_PIN_RESET;
}

int ocre_gpio_pin_toggle(int pin) {
    gpio_pin_ocre *gpio = get_gpio_from_id(pin);

    if (!gpio_system_initialized) {
        return -ENODEV;
    }

    if (!gpio || !gpio->in_use) {
        LOG_ERR("Invalid or unconfigured GPIO pin: %d", pin);
        return -EINVAL;
    }

    if (gpio->direction != OCRE_GPIO_DIR_OUTPUT) {
        LOG_ERR("Cannot toggle state of input pin: %d", pin);
        return -EINVAL;
    }

    int ret = gpio_pin_toggle(gpio->port, gpio->pin_number);
    if (ret != 0) {
        LOG_ERR("Failed to toggle GPIO pin %d: %d", pin, ret);
        return ret;
    }

    return 0;
}

int ocre_gpio_register_callback(int pin, ocre_gpio_callback_t callback) {
    gpio_pin_ocre *gpio = get_gpio_from_id(pin);

    if (!gpio_system_initialized) {
        return -ENODEV;
    }

    if (!gpio || !gpio->in_use) {
        LOG_ERR("Invalid or unconfigured GPIO pin: %d", pin);
        return -EINVAL;
    }

    if (gpio->direction != OCRE_GPIO_DIR_INPUT) {
        LOG_ERR("Cannot register callback on output pin: %d", pin);
        return -EINVAL;
    }

    // Configure the interrupt
    int ret = gpio_pin_interrupt_configure(gpio->port, gpio->pin_number, GPIO_INT_EDGE_BOTH);
    if (ret != 0) {
        LOG_ERR("Failed to configure GPIO interrupt: %d", ret);
        return ret;
    }

    // Initialize the callback structure
    gpio_init_callback(&gpio->gpio_cb_data, gpio_callback_handler, BIT(gpio->pin_number));
    ret = gpio_add_callback(gpio->port, &gpio->gpio_cb_data);
    if (ret != 0) {
        LOG_ERR("Failed to add GPIO callback: %d", ret);
        return ret;
    }

    // Store the user callback
    gpio->user_callback = callback;
    LOG_DBG("Registered callback for GPIO pin %d", pin);

    return 0;
}

int ocre_gpio_unregister_callback(int pin) {
    gpio_pin_ocre *gpio = get_gpio_from_id(pin);

    if (!gpio_system_initialized) {
        return -ENODEV;
    }

    if (!gpio || !gpio->in_use) {
        LOG_ERR("Invalid or unconfigured GPIO pin: %d", pin);
        return -EINVAL;
    }

    if (!gpio->user_callback) {
        LOG_WRN("No callback registered for pin %d", pin);
        return 0;
    }

    // Disable interrupt and remove callback
    int ret = gpio_pin_interrupt_configure(gpio->port, gpio->pin_number, GPIO_INT_DISABLE);
    if (ret != 0) {
        LOG_ERR("Failed to disable GPIO interrupt: %d", ret);
        return ret;
    }

    ret = gpio_remove_callback(gpio->port, &gpio->gpio_cb_data);
    if (ret != 0) {
        LOG_ERR("Failed to remove GPIO callback: %d", ret);
        return ret;
    }

    gpio->user_callback = NULL;
    LOG_DBG("Unregistered callback for GPIO pin %d", pin);

    return 0;
}

int get_pin_id(int port, int pin) {
    // Ensure the pin is valid within the specified port
    if (pin < 0 || pin >= CONFIG_OCRE_GPIO_PINS_PER_PORT) {
        printf("Invalid pin number\n");
        return -1; // Return -1 for invalid pin number
    }

    // Calculate the pin ID
    int pin_id = port * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    return pin_id;
}

// WASM-exposed functions for GPIO control
int ocre_gpio_wasm_init(wasm_exec_env_t exec_env) {
    return ocre_gpio_init();
}

int ocre_gpio_wasm_configure(wasm_exec_env_t exec_env, int port, int P_pin, int direction) {
    if (!gpio_system_initialized) {
        LOG_ERR("GPIO system not initialized");
        return -ENODEV;
    }
    int pin = get_pin_id(port, P_pin);
    ocre_gpio_config_t config;
    config.pin = pin;
    config.direction = direction;

    return ocre_gpio_configure(&config);
}

int ocre_gpio_wasm_set(wasm_exec_env_t exec_env, int port, int P_pin, int state) {
    int pin = get_pin_id(port, P_pin);
    return ocre_gpio_pin_set(pin, state ? OCRE_GPIO_PIN_SET : OCRE_GPIO_PIN_RESET);
}

int ocre_gpio_wasm_get(wasm_exec_env_t exec_env, int port, int P_pin) {
    int pin = get_pin_id(port, P_pin);
    return ocre_gpio_pin_get(pin);
}

int ocre_gpio_wasm_toggle(wasm_exec_env_t exec_env, int port, int P_pin) {
    int pin = get_pin_id(port, P_pin);
    return ocre_gpio_pin_toggle(pin);
}

int ocre_gpio_wasm_register_callback(wasm_exec_env_t exec_env, int port, int P_pin) {
    int pin = get_pin_id(port, P_pin);
    ocre_gpio_set_dispatcher(exec_env);

    // For WASM, we don't need an actual callback function pointer
    // as we'll use the dispatcher to call the appropriate WASM function
    return ocre_gpio_register_callback(pin, NULL);
}

int ocre_gpio_wasm_unregister_callback(wasm_exec_env_t exec_env, int port, int P_pin) {
    int pin = get_pin_id(port, P_pin);
    return ocre_gpio_unregister_callback(pin);
}
