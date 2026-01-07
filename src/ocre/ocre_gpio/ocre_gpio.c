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

typedef struct {
    const char *name;
    const struct device *port;
    gpio_pin_t pin;
    int port_idx;
} gpio_alias_t;
static gpio_alias_t gpio_aliases[CONFIG_OCRE_GPIO_MAX_PINS];
static int gpio_alias_count = 0;


static gpio_pin_ocre gpio_pins[CONFIG_OCRE_GPIO_MAX_PINS];
static const struct device *gpio_ports[CONFIG_OCRE_GPIO_MAX_PORTS];
static bool port_ready[CONFIG_OCRE_GPIO_MAX_PORTS];
static bool gpio_system_initialized = false;

#define INIT_GPIO_PORT_NAMED(idx, label, name_str)                                 \
    do {                                                                           \
        if (DT_NODE_EXISTS(label)) {                                               \
            gpio_ports[idx] = DEVICE_DT_GET(label);                                \
            if (!device_is_ready(gpio_ports[idx])) {                               \
                LOG_ERR("%s not ready", name_str);                                 \
            } else {                                                               \
                LOG_DBG("%s initialized", name_str);                               \
                port_ready[idx] = true;                                            \
                port_count++;                                                      \
            }                                                                      \
        }                                                                          \
    } while (0)

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
    INIT_GPIO_PORT_NAMED(0, DT_NODELABEL(gpioa), "GPIOA");
    INIT_GPIO_PORT_NAMED(1, DT_NODELABEL(gpiob), "GPIOB");
    INIT_GPIO_PORT_NAMED(2, DT_NODELABEL(gpioc), "GPIOC");
    INIT_GPIO_PORT_NAMED(3, DT_NODELABEL(gpiod), "GPIOD");
    INIT_GPIO_PORT_NAMED(4, DT_NODELABEL(gpioe), "GPIOE");
    INIT_GPIO_PORT_NAMED(5, DT_NODELABEL(gpiof), "GPIOF");
    INIT_GPIO_PORT_NAMED(6, DT_NODELABEL(gpiog), "GPIOG");
    INIT_GPIO_PORT_NAMED(7, DT_NODELABEL(gpioh), "GPIOH");

#elif defined(CONFIG_BOARD_ESP32C3_DEVKITM)
    INIT_GPIO_PORT_NAMED(0, DT_NODELABEL(gpio0), "GPIO0");

#elif defined(CONFIG_BOARD_MAX32690EVKIT)
    INIT_GPIO_PORT_NAMED(0, DT_NODELABEL(gpio0), "GPIO0");
    INIT_GPIO_PORT_NAMED(1, DT_NODELABEL(gpio1), "GPIO1");
    INIT_GPIO_PORT_NAMED(2, DT_NODELABEL(gpio2), "GPIO2");
    INIT_GPIO_PORT_NAMED(3, DT_NODELABEL(gpio3), "GPIO3");
    INIT_GPIO_PORT_NAMED(4, DT_NODELABEL(gpio4), "GPIO4");

#elif defined(CONFIG_BOARD_ARDUINO_PORTENTA_H7)
    INIT_GPIO_PORT_NAMED(0,  DT_NODELABEL(gpioa), "GPIOA");
    INIT_GPIO_PORT_NAMED(1,  DT_NODELABEL(gpiob), "GPIOB");
    INIT_GPIO_PORT_NAMED(2,  DT_NODELABEL(gpioc), "GPIOC");
    INIT_GPIO_PORT_NAMED(3,  DT_NODELABEL(gpiod), "GPIOD");
    INIT_GPIO_PORT_NAMED(4,  DT_NODELABEL(gpioe), "GPIOE");
    INIT_GPIO_PORT_NAMED(5,  DT_NODELABEL(gpiof), "GPIOF");
    INIT_GPIO_PORT_NAMED(6,  DT_NODELABEL(gpiog), "GPIOG");
    INIT_GPIO_PORT_NAMED(7,  DT_NODELABEL(gpioh), "GPIOH");
    INIT_GPIO_PORT_NAMED(8,  DT_NODELABEL(gpioi), "GPIOI");
    INIT_GPIO_PORT_NAMED(9,  DT_NODELABEL(gpioj), "GPIOJ");
    INIT_GPIO_PORT_NAMED(10, DT_NODELABEL(gpiok), "GPIOK");

#elif defined(CONFIG_BOARD_W5500_EVB_PICO2)
    INIT_GPIO_PORT_NAMED(0, DT_NODELABEL(gpio0), "GPIO0");

#else
    // Generic fallback
#if DT_NODE_EXISTS(DT_NODELABEL(gpio0))
    INIT_GPIO_PORT_NAMED(0, DT_NODELABEL(gpio0), "GPIO0");
#elif DT_NODE_EXISTS(DT_NODELABEL(gpioa))
    INIT_GPIO_PORT_NAMED(0,  DT_NODELABEL(gpioa), "GPIOA");
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
            LOG_DBG("Cleaned up GPIO pin %d", i);
        }
    }
    LOG_DBG("Cleaned up GPIO resources for module %p", (void *)module_inst);
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

            ocre_event_t event;
            event.type = OCRE_RESOURCE_TYPE_GPIO;
            event.data.gpio_event.pin_id = gpio_pins[i].pin_number;
            event.data.gpio_event.port = gpio_pins[i].port_idx;
            event.data.gpio_event.state = (uint32_t)state;
            event.owner = gpio_pins[i].owner;
            core_spinlock_key_t key = core_spinlock_lock(&ocre_event_queue_lock);
            if (core_eventq_put(&ocre_event_queue, &event) != 0) {
                LOG_ERR("Failed to queue GPIO event for pin %d", i);
            } else {
                LOG_INF("Queued GPIO event for pin %d (port=%d, pin=%d), state=%d", i, gpio_pins[i].port_idx,
                        gpio_pins[i].pin_number, state);
            }
            core_spinlock_unlock(&ocre_event_queue_lock, key);
            }
        }
    }
}

//========================================================================================================================================================================================================================================================================================================
// By Name
//========================================================================================================================================================================================================================================================================================================
static int find_port_index(const struct device *port) {
    if (!port) {
        LOG_ERR("Null port provided to find_port_index");
        return -EINVAL;
    }
    for (int i = 0; i < CONFIG_OCRE_GPIO_MAX_PORTS; i++) {
        if (port_ready[i] && gpio_ports[i] == port) {
            LOG_DBG("Found port at index %d: %p", i, port);
            return i;
        }
    }
    LOG_ERR("Port %p not found in initialized ports", port);
    return -1;
}

static int resolve_gpio_alias(const char *name, int *port_idx, gpio_pin_t *pin) {
    if (!name || !port_idx || !pin) {
        LOG_ERR("Invalid parameters: name=%p, port_idx=%p, pin=%p", name, port_idx, pin);
        return -EINVAL;
    }

    // Check if GPIO system is initialized
    if (!gpio_system_initialized) {
        LOG_ERR("GPIO system not initialized when resolving alias '%s'", name);
        return -ENODEV;
    }

    // First check if we already have this alias cached
    for (int i = 0; i < gpio_alias_count; i++) {
        if (strcmp(gpio_aliases[i].name, name) == 0) {
            *port_idx = gpio_aliases[i].port_idx;
            *pin = gpio_aliases[i].pin;
            LOG_INF("Found cached GPIO alias '%s': port %d, pin %d", name, *port_idx, *pin);
            return 0;
        }
    }

    // Try to resolve the alias using devicetree
    const struct gpio_dt_spec *spec = NULL;
    static struct gpio_dt_spec gpio_spec;

    // Check common aliases - only compile if they exist
    if (strcmp(name, "led0") == 0) {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
        gpio_spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
        spec = &gpio_spec;
        LOG_DBG("Resolved DT_ALIAS(led0) to port %p, pin %d", spec ? spec->port : NULL, spec ? spec->pin : 0);
#else
        LOG_ERR("DT_ALIAS(led0) not defined in device tree");
        return -ENODEV;
#endif
    } else if (strcmp(name, "led1") == 0) {
#if DT_NODE_EXISTS(DT_ALIAS(led1))
        gpio_spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
        spec = &gpio_spec;
        LOG_DBG("Resolved DT_ALIAS(led1) to port %p, pin %d", spec ? spec->port : NULL, spec ? spec->pin : 0);
#else
        LOG_ERR("DT_ALIAS(led1) not defined in device tree");
        return -ENODEV;
#endif
    } else if (strcmp(name, "led2") == 0) {
#if DT_NODE_EXISTS(DT_ALIAS(led2))
        gpio_spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
        spec = &gpio_spec;
        LOG_DBG("Resolved DT_ALIAS(led2) to port %p, pin %d", spec ? spec->port : NULL, spec ? spec->pin : 0);
#else
        LOG_ERR("DT_ALIAS(led2) not defined in device tree");
        return -ENODEV;
#endif
    } else if (strcmp(name, "sw0") == 0) {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
        gpio_spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
        spec = &gpio_spec;
        LOG_DBG("Resolved DT_ALIAS(sw0) to port %p, pin %d", spec ? spec->port : NULL, spec ? spec->pin : 0);
#else
        LOG_ERR("DT_ALIAS(sw0) not defined in device tree");
        return -ENODEV;
#endif
    } else if (strcmp(name, "sw1") == 0) {
#if DT_NODE_EXISTS(DT_ALIAS(sw1))
        gpio_spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
        spec = &gpio_spec;
        LOG_DBG("Resolved DT_ALIAS(sw1) to port %p, pin %d", spec ? spec->port : NULL, spec ? spec->pin : 0);
#else
        LOG_ERR("DT_ALIAS(sw1) not defined in device tree");
        return -ENODEV;
#endif
    } else {
        LOG_ERR("Unknown GPIO alias '%s'", name);
        return -EINVAL;
    }

    if (!spec || !spec->port) {
        LOG_ERR("GPIO alias '%s' not found in device tree or invalid spec", name);
        return -ENODEV;
    }

    // Verify device readiness
    if (!device_is_ready(spec->port)) {
        LOG_ERR("Device for GPIO alias '%s' (port %p) not ready", name, spec->port);
        return -ENODEV;
    }

    // Find the port index
    int found_port_idx = find_port_index(spec->port);
    if (found_port_idx < 0) {
        LOG_ERR("Port for alias '%s' (port %p) not found in initialized ports", name, spec->port);
        // Debug: print all available ports
        LOG_ERR("Available ports:");
        bool any_ports = false;
        for (int i = 0; i < CONFIG_OCRE_GPIO_MAX_PORTS; i++) {
            if (port_ready[i] && gpio_ports[i]) {
                LOG_ERR("  Port %d: %p", i, gpio_ports[i]);
                any_ports = true;
            }
        }
        if (!any_ports) {
            LOG_ERR("  No ports initialized");
        }
        LOG_ERR("Looking for port: %p", spec->port);
        return -ENODEV;
    }

    *port_idx = found_port_idx;
    *pin = spec->pin;

    // Cache the resolved alias
    if (gpio_alias_count < CONFIG_OCRE_GPIO_MAX_PINS) {
        gpio_aliases[gpio_alias_count].name = name;
        gpio_aliases[gpio_alias_count].port = spec->port;
        gpio_aliases[gpio_alias_count].pin = spec->pin;
        gpio_aliases[gpio_alias_count].port_idx = found_port_idx;
        gpio_alias_count++;
        LOG_INF("Cached GPIO alias '%s': port %d, pin %d", name, found_port_idx, spec->pin);
    } else {
        LOG_WRN("Cannot cache alias '%s': alias table full", name);
    }

    LOG_INF("Resolved GPIO alias '%s' to port %d, pin %d", name, *port_idx, *pin);
    return 0;
}

int ocre_gpio_configure_by_name(const char *name, ocre_gpio_direction_t direction) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    int port_idx;
    gpio_pin_t pin;
    int ret = resolve_gpio_alias(name, &port_idx, &pin);
    if (ret != 0) {
        return ret;
    }

    ocre_gpio_config_t config = {
        .pin = pin,
        .port_idx = port_idx,
        .direction = direction
    };

    return ocre_gpio_configure(&config);
}

int ocre_gpio_set_by_name(const char *name, ocre_gpio_pin_state_t state) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    int port_idx;
    gpio_pin_t pin;
    int ret = resolve_gpio_alias(name, &port_idx, &pin);
    if (ret != 0) {
        return ret;
    }

    int global_pin = port_idx * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    return ocre_gpio_pin_set(global_pin, state);
}

int ocre_gpio_toggle_by_name(const char *name) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    int port_idx;
    gpio_pin_t pin;
    int ret = resolve_gpio_alias(name, &port_idx, &pin);
    if (ret != 0) {
        return ret;
    }

    int global_pin = port_idx * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    return ocre_gpio_pin_toggle(global_pin);
}

ocre_gpio_pin_state_t ocre_gpio_get_by_name(const char *name) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    int port_idx;
    gpio_pin_t pin;
    int ret = resolve_gpio_alias(name, &port_idx, &pin);
    if (ret != 0) {
        return ret;
    }

    int global_pin = port_idx * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    return ocre_gpio_pin_get(global_pin);
}

int ocre_gpio_wasm_configure_by_name(wasm_exec_env_t exec_env, const char *name, int direction) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        LOG_ERR("No module instance for GPIO configuration");
        return -EINVAL;
    }

    LOG_INF("Configuring GPIO by name: %s, direction=%d", name, direction);
    return ocre_gpio_configure_by_name(name, direction);
}

int ocre_gpio_wasm_set_by_name(wasm_exec_env_t exec_env, const char *name, int state) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    LOG_INF("Setting GPIO by name: %s, state=%d", name, state);
    return ocre_gpio_set_by_name(name, state ? OCRE_GPIO_PIN_SET : OCRE_GPIO_PIN_RESET);
}

int ocre_gpio_wasm_get_by_name(wasm_exec_env_t exec_env, const char *name) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    LOG_INF("Getting GPIO by name: %s", name);
    return ocre_gpio_get_by_name(name);
}

int ocre_gpio_wasm_toggle_by_name(wasm_exec_env_t exec_env, const char *name) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    LOG_INF("Toggling GPIO by name: %s", name);
    return ocre_gpio_toggle_by_name(name);
}

int ocre_gpio_wasm_register_callback_by_name(wasm_exec_env_t exec_env, const char *name) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    int port_idx;
    gpio_pin_t pin;
    int ret = resolve_gpio_alias(name, &port_idx, &pin);
    if (ret != 0) {
        return ret;
    }

    int global_pin = port_idx * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    LOG_INF("Registering callback by name: %s, global_pin=%d", name, global_pin);

    if (global_pin >= CONFIG_OCRE_GPIO_MAX_PINS) {
        LOG_ERR("Global pin %d exceeds max %d", global_pin, CONFIG_OCRE_GPIO_MAX_PINS);
        return -EINVAL;
    }

    return ocre_gpio_register_callback(global_pin);
}

int ocre_gpio_wasm_unregister_callback_by_name(wasm_exec_env_t exec_env, const char *name) {
    if (!name) {
        LOG_ERR("Invalid name parameter");
        return -EINVAL;
    }

    int port_idx;
    gpio_pin_t pin;
    int ret = resolve_gpio_alias(name, &port_idx, &pin);
    if (ret != 0) {
        return ret;
    }

    int global_pin = port_idx * CONFIG_OCRE_GPIO_PINS_PER_PORT + pin;
    LOG_INF("Unregistering callback by name: %s, global_pin=%d", name, global_pin);

    return ocre_gpio_unregister_callback(global_pin);
}
