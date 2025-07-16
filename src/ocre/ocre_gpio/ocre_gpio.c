/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre, a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <ocre/api/ocre_common.h>
#include <ocre/ocre_gpio/ocre_gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

typedef struct {
    uint32_t in_use: 1;
    uint32_t direction: 1;
    uint32_t pin_number: 6;
    uint32_t port_idx: 4;
    struct gpio_callback *cb;
    wasm_module_inst_t owner;
} gpio_pin_ocre;

static gpio_pin_ocre gpio_pins[CONFIG_OCRE_GPIO_MAX_PINS];
static const struct device *gpio_ports[CONFIG_OCRE_GPIO_MAX_PORTS];
static bool port_ready[CONFIG_OCRE_GPIO_MAX_PORTS];
static bool gpio_system_initialized = false;
extern struct k_msgq wasm_event_queue;
extern struct k_spinlock wasm_event_queue_lock;

#if defined(CONFIG_BOARD_B_U585I_IOT02A)
#define GPIO_PORT_A DT_NODELABEL(gpioa)
#define GPIO_PORT_B DT_NODELABEL(gpiob)
#define GPIO_PORT_C DT_NODELABEL(gpioc)
#define GPIO_PORT_D DT_NODELABEL(gpiod)
#define GPIO_PORT_E DT_NODELABEL(gpioe)
#define GPIO_PORT_F DT_NODELABEL(gpiof)
#define GPIO_PORT_G DT_NODELABEL(gpiog)
#define GPIO_PORT_H DT_NODELABEL(gpioh)
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM)
#define GPIO_PORT_0 DT_NODELABEL(gpio0)
#else
#if DT_NODE_EXISTS(DT_NODELABEL(gpio0))
#define GPIO_PORT_0 DT_NODELABEL(gpio0)
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(gpioa))
#define GPIO_PORT_A DT_NODELABEL(gpioa)
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(gpio))
#define GPIO_PORT_G DT_NODELABEL(gpio)
#endif
#endif

static void gpio_callback_handler(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins);

int ocre_gpio_init(void) {
    if (gpio_system_initialized) {
        LOG_INF("GPIO system already initialized");
        return 0;
    }
    if (!common_initialized && ocre_common_init() != 0) {
        LOG_ERR("Failed to initialize common subsystem");
        return -EINVAL;
    }
    int port_count = 0;
    memset(port_ready, 0, sizeof(port_ready));
#if defined(CONFIG_BOARD_B_U585I_IOT02A)
    if (DT_NODE_EXISTS(GPIO_PORT_A)) {
        gpio_ports[0] = DEVICE_DT_GET(GPIO_PORT_A);
        if (!device_is_ready(gpio_ports[0])) {
            LOG_ERR("GPIOA is not ready");
        } else {
            LOG_INF("GPIOA is initialized");
            port_ready[0] = true;
            port_count++;
        }
    }
    if (DT_NODE_EXISTS(GPIO_PORT_B)) {
        gpio_ports[1] = DEVICE_DT_GET(GPIO_PORT_B);
        if (!device_is_ready(gpio_ports[1])) {
            LOG_ERR("GPIOB is not ready");
        } else {
            LOG_INF("GPIOB is initialized");
            port_ready[1] = true;
            port_count++;
        }
    }
    if (DT_NODE_EXISTS(GPIO_PORT_C)) {
        gpio_ports[2] = DEVICE_DT_GET(GPIO_PORT_C);
        if (!device_is_ready(gpio_ports[2])) {
            LOG_ERR("GPIOC is not ready");
        } else {
            LOG_INF("GPIOC is initialized");
            port_ready[2] = true;
            port_count++;
        }
    }
    if (DT_NODE_EXISTS(GPIO_PORT_D)) {
        gpio_ports[3] = DEVICE_DT_GET(GPIO_PORT_D);
        if (!device_is_ready(gpio_ports[3])) {
            LOG_ERR("GPIOD is not ready");
        } else {
            LOG_INF("GPIOD is initialized");
            port_ready[3] = true;
            port_count++;
        }
    }
    if (DT_NODE_EXISTS(GPIO_PORT_E)) {
        gpio_ports[4] = DEVICE_DT_GET(GPIO_PORT_E);
        if (!device_is_ready(gpio_ports[4])) {
            LOG_ERR("GPIOE is not ready");
        } else {
            LOG_INF("GPIOE is initialized");
            port_ready[4] = true;
            port_count++;
        }
    }
    if (DT_NODE_EXISTS(GPIO_PORT_F)) {
        gpio_ports[5] = DEVICE_DT_GET(GPIO_PORT_F);
        if (!device_is_ready(gpio_ports[5])) {
            LOG_ERR("GPIOF is not ready");
        } else {
            LOG_INF("GPIOF is initialized");
            port_ready[5] = true;
            port_count++;
        }
    }
    if (DT_NODE_EXISTS(GPIO_PORT_G)) {
        gpio_ports[6] = DEVICE_DT_GET(GPIO_PORT_G);
        if (!device_is_ready(gpio_ports[6])) {
            LOG_ERR("GPIOG is not ready");
        } else {
            LOG_INF("GPIOG is initialized");
            port_ready[6] = true;
            port_count++;
        }
    }
    if (DT_NODE_EXISTS(GPIO_PORT_H)) {
        gpio_ports[7] = DEVICE_DT_GET(GPIO_PORT_H);
        if (!device_is_ready(gpio_ports[7])) {
            LOG_ERR("GPIOH is not ready");
        } else {
            LOG_INF("GPIOH is initialized");
            port_ready[7] = true;
            port_count++;
        }
    }
#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM)
    if (DT_NODE_EXISTS(GPIO_PORT_0)) {
        gpio_ports[0] = DEVICE_DT_GET(GPIO_PORT_0);
        if (!device_is_ready(gpio_ports[0])) {
            LOG_ERR("GPIO0 is not ready");
        } else {
            LOG_INF("GPIO0 is initialized");
            port_ready[0] = true;
            port_count++;
        }
    }
#else
#if defined(GPIO_PORT_A) && DT_NODE_EXISTS(GPIO_PORT_A)
    gpio_ports[0] = DEVICE_DT_GET(GPIO_PORT_A);
    if (!device_is_ready(gpio_ports[0])) {
        LOG_ERR("GPIOA is not ready");
    } else {
        LOG_INF("GPIOA is initialized");
        port_ready[0] = true;
        port_count++;
    }
#endif
#if defined(GPIO_PORT_G) && DT_NODE_EXISTS(GPIO_PORT_G)
    gpio_ports[0] = DEVICE_DT_GET(GPIO_PORT_G);
    if (!device_is_ready(gpio_ports[0])) {
        LOG_ERR("GPIO is not ready");
    } else {
        LOG_INF("GPIO is initialized");
        port_ready[0] = true;
        port_count++;
    }
#endif
#endif
    if (port_count == 0) {
        LOG_ERR("No GPIO ports were initialized");
        return -ENODEV;
    }
    ocre_register_cleanup_handler(OCRE_RESOURCE_TYPE_GPIO, ocre_gpio_cleanup_container);
    gpio_system_initialized = true;
    LOG_INF("GPIO system initialized, %d ports available", port_count);
    return 0;
}

int ocre_gpio_configure(const ocre_gpio_config_t *config) {
    if (!gpio_system_initialized || !config || config->pin >= CONFIG_OCRE_GPIO_PINS_PER_PORT ||
        config->port_idx >= CONFIG_OCRE_GPIO_MAX_PORTS || !port_ready[config->port_idx]) {
        LOG_ERR("Invalid GPIO config: port=%d, pin=%d, port_ready=%d", config ? config->port_idx : -1,
                config ? config->pin : -1, config ? port_ready[config->port_idx] : false);
        return -EINVAL;
    }
    int port_idx = config->port_idx;
    gpio_pin_t pin = config->pin;
    gpio_flags_t flags = (config->direction == OCRE_GPIO_DIR_INPUT) ? GPIO_INPUT : GPIO_OUTPUT;
    if (config->direction == OCRE_GPIO_DIR_INPUT) {
        flags |= GPIO_PULL_UP;
    }
    int ret = gpio_pin_configure(gpio_ports[port_idx], pin, flags);
    if (ret != 0) {
        LOG_ERR("Failed to configure GPIO pin %d on port %d: %d", pin, port_idx, ret);
        return ret;
    }
    wasm_module_inst_t module_inst = ocre_get_current_module();
    if (!module_inst) {
        LOG_ERR("No current module instance for GPIO configuration");
        return -EINVAL;
    }
    int global_pin = port_idx * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    if (global_pin >= CONFIG_OCRE_GPIO_MAX_PINS) {
        LOG_ERR("Global pin %d exceeds max %d", global_pin, CONFIG_OCRE_GPIO_MAX_PINS);
        return -EINVAL;
    }
    gpio_pins[global_pin] = (gpio_pin_ocre){.in_use = 1,
                                            .direction = (config->direction == OCRE_GPIO_DIR_OUTPUT),
                                            .pin_number = pin,
                                            .port_idx = port_idx,
                                            .cb = NULL,
                                            .owner = module_inst};
    ocre_increment_resource_count(module_inst, OCRE_RESOURCE_TYPE_GPIO);
    LOG_INF("Configured GPIO pin %d on port %d (global %d) for module %p", pin, port_idx, global_pin,
            (void *)module_inst);
    return 0;
}

int ocre_gpio_pin_set(int pin, ocre_gpio_pin_state_t state) {
    if (pin >= CONFIG_OCRE_GPIO_MAX_PINS || !gpio_pins[pin].in_use || gpio_pins[pin].direction != 1 ||
        !port_ready[gpio_pins[pin].port_idx]) {
        LOG_ERR("Invalid GPIO pin %d or not configured as output or port not ready", pin);
        return -EINVAL;
    }
    int ret = gpio_pin_set(gpio_ports[gpio_pins[pin].port_idx], gpio_pins[pin].pin_number, state);
    if (ret != 0) {
        LOG_ERR("Failed to set GPIO pin %d: %d", pin, ret);
    }
    return ret;
}

ocre_gpio_pin_state_t ocre_gpio_pin_get(int pin) {
    if (pin >= CONFIG_OCRE_GPIO_MAX_PINS || !gpio_pins[pin].in_use || !port_ready[gpio_pins[pin].port_idx]) {
        LOG_ERR("Invalid or unconfigured GPIO pin %d or port not ready", pin);
        return -EINVAL;
    }
    int value = gpio_pin_get(gpio_ports[gpio_pins[pin].port_idx], gpio_pins[pin].pin_number);
    return (value >= 0) ? (value ? OCRE_GPIO_PIN_SET : OCRE_GPIO_PIN_RESET) : value;
}

int ocre_gpio_pin_toggle(int pin) {
    if (pin >= CONFIG_OCRE_GPIO_MAX_PINS || !gpio_pins[pin].in_use || gpio_pins[pin].direction != 1 ||
        !port_ready[gpio_pins[pin].port_idx]) {
        LOG_ERR("Invalid GPIO pin %d or not configured as output or port not ready", pin);
        return -EINVAL;
    }
    int ret = gpio_pin_toggle(gpio_ports[gpio_pins[pin].port_idx], gpio_pins[pin].pin_number);
    if (ret != 0) {
        LOG_ERR("Failed to toggle GPIO pin %d: %d", pin, ret);
    }
    return ret;
}

int ocre_gpio_register_callback(int pin) {
    if (pin >= CONFIG_OCRE_GPIO_MAX_PINS || !gpio_pins[pin].in_use || gpio_pins[pin].direction != 0 ||
        !port_ready[gpio_pins[pin].port_idx]) {
        LOG_ERR("Invalid GPIO pin %d or not configured as input or port not ready", pin);
        return -EINVAL;
    }
    gpio_pin_ocre *gpio = &gpio_pins[pin];
    if (!gpio->cb) {
        gpio->cb = k_calloc(1, sizeof(struct gpio_callback));
        if (!gpio->cb) {
            LOG_ERR("Failed to allocate memory for GPIO callback");
            return -ENOMEM;
        }
    }
    int ret = gpio_pin_interrupt_configure(gpio_ports[gpio->port_idx], gpio->pin_number, GPIO_INT_EDGE_BOTH);
    if (ret) {
        LOG_ERR("Failed to configure interrupt for GPIO pin %d: %d", pin, ret);
        k_free(gpio->cb);
        gpio->cb = NULL;
        return ret;
    }
    gpio_init_callback(gpio->cb, gpio_callback_handler, BIT(gpio->pin_number));
    ret = gpio_add_callback(gpio_ports[gpio->port_idx], gpio->cb);
    if (ret) {
        LOG_ERR("Failed to add callback for GPIO pin %d: %d", pin, ret);
        k_free(gpio->cb);
        gpio->cb = NULL;
        return ret;
    }
    LOG_INF("Registered callback for GPIO pin %d (port %d, pin %d)", pin, gpio->port_idx, gpio->pin_number);
    return 0;
}

int ocre_gpio_unregister_callback(int pin) {
    if (pin >= CONFIG_OCRE_GPIO_MAX_PINS || !gpio_pins[pin].in_use || !gpio_pins[pin].cb ||
        !port_ready[gpio_pins[pin].port_idx]) {
        LOG_ERR("Invalid GPIO pin %d or no callback registered or port not ready", pin);
        return -EINVAL;
    }
    gpio_pin_ocre *gpio = &gpio_pins[pin];
    int ret = gpio_pin_interrupt_configure(gpio_ports[gpio->port_idx], gpio->pin_number, GPIO_INT_DISABLE);
    if (ret) {
        LOG_ERR("Failed to disable interrupt for GPIO pin %d: %d", pin, ret);
        return ret;
    }
    ret = gpio_remove_callback(gpio_ports[gpio->port_idx], gpio->cb);
    if (ret) {
        LOG_ERR("Failed to remove callback for GPIO pin %d: %d", pin, ret);
    }
    k_free(gpio->cb);
    gpio->cb = NULL;
    LOG_INF("Unregistered callback for GPIO pin %d", pin);
    return ret;
}

void ocre_gpio_cleanup_container(wasm_module_inst_t module_inst) {
    if (!gpio_system_initialized || !module_inst) {
        LOG_ERR("GPIO system not initialized or invalid module %p", (void *)module_inst);
        return;
    }
    for (int i = 0; i < CONFIG_OCRE_GPIO_MAX_PINS; i++) {
        if (gpio_pins[i].in_use && gpio_pins[i].owner == module_inst) {
            if (gpio_pins[i].direction == 0) {
                ocre_gpio_unregister_callback(i);
            }
            gpio_pins[i].in_use = 0;
            gpio_pins[i].owner = NULL;
            ocre_decrement_resource_count(module_inst, OCRE_RESOURCE_TYPE_GPIO);
            LOG_INF("Cleaned up GPIO pin %d", i);
        }
    }
    LOG_INF("Cleaned up GPIO resources for module %p", (void *)module_inst);
}

void ocre_gpio_register_module(wasm_module_inst_t module_inst) {
    if (module_inst) {
        ocre_register_module(module_inst);
        LOG_INF("Registered GPIO module %p", (void *)module_inst);
    }
}

void ocre_gpio_set_dispatcher(wasm_exec_env_t exec_env) {
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (module_inst) {
        // Dispatcher set in wasm container application
        LOG_INF("Set dispatcher for module %p", (void *)module_inst);
    }
}

int ocre_gpio_wasm_init(wasm_exec_env_t exec_env) {
    return ocre_gpio_init();
}

int ocre_gpio_wasm_configure(wasm_exec_env_t exec_env, int port, int pin, int direction) {
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        LOG_ERR("No module instance for GPIO configuration");
        return -EINVAL;
    }
    if (port >= CONFIG_OCRE_GPIO_MAX_PORTS || pin >= CONFIG_OCRE_GPIO_PINS_PER_PORT || !port_ready[port]) {
        LOG_ERR("Invalid port=%d, pin=%d, or port not ready", port, pin);
        return -EINVAL;
    }
    int global_pin = port * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    LOG_INF("Configuring GPIO: port=%d, pin=%d, global_pin=%d, direction=%d", port, pin, global_pin, direction);
    if (global_pin >= CONFIG_OCRE_GPIO_MAX_PINS) {
        LOG_ERR("Global pin %d exceeds max %d", global_pin, CONFIG_OCRE_GPIO_MAX_PINS);
        return -EINVAL;
    }
    ocre_gpio_config_t config = {.pin = pin, .port_idx = port, .direction = direction};
    return ocre_gpio_configure(&config);
}

int ocre_gpio_wasm_set(wasm_exec_env_t exec_env, int port, int pin, int state) {
    int global_pin = port * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    LOG_INF("Setting GPIO: port=%d, pin=%d, global_pin=%d, state=%d", port, pin, global_pin, state);
    if (!port_ready[port]) {
        LOG_ERR("Port %d not ready", port);
        return -EINVAL;
    }
    return ocre_gpio_pin_set(global_pin, state ? OCRE_GPIO_PIN_SET : OCRE_GPIO_PIN_RESET);
}

int ocre_gpio_wasm_get(wasm_exec_env_t exec_env, int port, int pin) {
    int global_pin = port * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    LOG_INF("Getting GPIO: port=%d, pin=%d, global_pin=%d", port, pin, global_pin);
    if (!port_ready[port]) {
        LOG_ERR("Port %d not ready", port);
        return -EINVAL;
    }
    return ocre_gpio_pin_get(global_pin);
}

int ocre_gpio_wasm_toggle(wasm_exec_env_t exec_env, int port, int pin) {
    int global_pin = port * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    LOG_INF("Toggling GPIO: port=%d, pin=%d, global_pin=%d", port, pin, global_pin);
    if (!port_ready[port]) {
        LOG_ERR("Port %d not ready", port);
        return -EINVAL;
    }
    return ocre_gpio_pin_toggle(global_pin);
}

int ocre_gpio_wasm_register_callback(wasm_exec_env_t exec_env, int port, int pin) {
    int global_pin = port * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    LOG_INF("Registering callback: port=%d, pin=%d, global_pin=%d", port, pin, global_pin);
    if (global_pin >= CONFIG_OCRE_GPIO_MAX_PINS || !port_ready[port]) {
        LOG_ERR("Global pin %d exceeds max %d or port %d not ready", global_pin, CONFIG_OCRE_GPIO_MAX_PINS, port);
        return -EINVAL;
    }
    return ocre_gpio_register_callback(global_pin);
}

int ocre_gpio_wasm_unregister_callback(wasm_exec_env_t exec_env, int port, int pin) {
    int global_pin = port * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    LOG_INF("Unregistering callback: port=%d, pin=%d, global_pin=%d", port, pin, global_pin);
    if (!port_ready[port]) {
        LOG_ERR("Port %d not ready", port);
        return -EINVAL;
    }
    return ocre_gpio_unregister_callback(global_pin);
}

static void gpio_callback_handler(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
    if (!port || !cb) {
        LOG_ERR("Null port or callback in GPIO handler");
        return;
    }
    for (int i = 0; i < CONFIG_OCRE_GPIO_MAX_PINS; i++) {
        if (gpio_pins[i].in_use && gpio_pins[i].cb == cb && (pins & BIT(gpio_pins[i].pin_number))) {
            int state = gpio_pin_get(port, gpio_pins[i].pin_number);
            if (state >= 0) {
                wasm_event_t event = {.type = OCRE_RESOURCE_TYPE_GPIO,
                                      .id = i,
                                      .port = gpio_pins[i].port_idx,
                                      .state = (uint32_t)state};
                k_spinlock_key_t key = k_spin_lock(&wasm_event_queue_lock);
                if (k_msgq_put(&wasm_event_queue, &event, K_NO_WAIT) != 0) {
                    LOG_ERR("Failed to queue GPIO event for pin %d", i);
                } else {
                    LOG_INF("Queued GPIO event for pin %d (port=%d, pin=%d), state=%d", i, gpio_pins[i].port_idx,
                            gpio_pins[i].pin_number, state);
                }
                k_spin_unlock(&wasm_event_queue_lock, key);
            }
        }
    }
}
