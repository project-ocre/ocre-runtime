/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre, a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include "ocre_core_external.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/mem_manage.h>
#include <zephyr/drivers/mm/system_mm.h>
#include <zephyr/sys/slist.h>
#include <stdlib.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);
#include <autoconf.h>
#include "../../../../../wasm-micro-runtime/core/iwasm/include/lib_export.h"
#include "bh_log.h"
#include "../ocre_timers/ocre_timer.h"
#include "../ocre_gpio/ocre_gpio.h"
#include "ocre_common.h"
#include <zephyr/sys/ring_buffer.h>

// Place queue buffer in a dedicated section with alignment
__attribute__((section(".noinit.wasm_event_queue"),
               aligned(8))) char wasm_event_queue_buffer[64 * sizeof(wasm_event_t)];

char *wasm_event_queue_buffer_ptr = wasm_event_queue_buffer; // Pointer for validation
K_MSGQ_DEFINE(wasm_event_queue, sizeof(wasm_event_t), 64, 4);
bool wasm_event_queue_initialized = false;
struct k_spinlock wasm_event_queue_lock;

#define EVENT_BUFFER_SIZE 512
static uint8_t event_buffer[EVENT_BUFFER_SIZE];
static struct ring_buf event_ring;

typedef struct module_node {
    ocre_module_context_t ctx;
    sys_snode_t node;
} module_node_t;

static sys_slist_t module_registry;
static struct k_mutex registry_mutex;

static struct cleanup_handler {
    ocre_resource_type_t type;
    ocre_cleanup_handler_t handler;
} cleanup_handlers[OCRE_RESOURCE_TYPE_COUNT];

/* Thread-Local Storage */
__thread wasm_module_inst_t *current_module_tls = NULL;

bool common_initialized = false;

static struct k_sem event_sem;

#define EVENT_THREAD_POOL_SIZE 2
static struct core_thread event_threads[EVENT_THREAD_POOL_SIZE];

// Flag to signal event threads to exit gracefully
static volatile bool event_threads_exit = false;

// Arguments for event threads
struct event_thread_args {
    int index; // Thread index for identification or logging
};

static struct event_thread_args event_args[EVENT_THREAD_POOL_SIZE];

// Event thread function to process events from the ring buffer
static void event_thread_fn(void *arg1) {
    struct event_thread_args *args = (struct event_thread_args *)arg1;
    int index = args->index; // Can be used for logging or debugging
    // Initialize WASM runtime thread environment
    wasm_runtime_init_thread_env();

    wasm_event_t event_batch[10];
    uint32_t bytes_read;

    // Main event processing loop
    while (!event_threads_exit) {
        k_sem_take(&event_sem, K_FOREVER);
        if (event_threads_exit) {
            break; // Exit if shutdown is signaled
        }
        bytes_read = ring_buf_get(&event_ring, (uint8_t *)event_batch, sizeof(event_buffer));
        for (size_t i = 0; i < bytes_read / sizeof(wasm_event_t); i++) {
            wasm_event_t *event = &event_batch[i];
            if (!event) {
                LOG_ERR("Null event in batch");
                continue;
            }
            if (event->type >= OCRE_RESOURCE_TYPE_COUNT) {
                LOG_ERR("Invalid event type: %d", event->type);
                continue;
            }
            wasm_module_inst_t module_inst = current_module_tls ? *current_module_tls : NULL;
            if (!module_inst) {
                LOG_ERR("No module instance for event type %d", event->type);
                continue;
            }
            switch (event->type) {
                case OCRE_RESOURCE_TYPE_TIMER:
                    LOG_INF("Timer event: id=%d, owner=%p", event->id, (void *)module_inst);
                    break;
                case OCRE_RESOURCE_TYPE_GPIO:
                    LOG_INF("GPIO event: id=%d, state=%d, owner=%p", event->id, event->state, (void *)module_inst);
                    break;
                case OCRE_RESOURCE_TYPE_SENSOR:
                    LOG_INF("Sensor event: id=%d, channel=%d, value=%d, owner=%p", event->id, event->port, event->state,
                            (void *)module_inst);
                    break;
                default:
                    LOG_ERR("Unhandled event type: %d", event->type);
                    continue;
            }
            k_mutex_lock(&registry_mutex, K_FOREVER);
            module_node_t *node;
            bool found = false;
            SYS_SLIST_FOR_EACH_CONTAINER(&module_registry, node, node) {
                if (node->ctx.inst == module_inst) {
                    found = true;
                    wasm_function_inst_t dispatcher = node->ctx.dispatchers[event->type];
                    if (!dispatcher) {
                        LOG_ERR("No dispatcher for event type %d, module %p", event->type, (void *)module_inst);
                        break;
                    }
                    if (!node->ctx.exec_env) {
                        LOG_ERR("Null exec_env for module %p", (void *)module_inst);
                        break;
                    }
                    // Array to store arguments for wasm_event_t handler. Size is 3 to hold:
                    // 1. Event type identifier
                    // 2. Event data pointer
                    // 3. Additional context or flags
                    uint32_t args[3] = {0};
                    bool result = false;
                    current_module_tls = &node->ctx.inst;
                    switch (event->type) {
                        case OCRE_RESOURCE_TYPE_TIMER:
                            args[0] = event->id;
                            LOG_INF("Dispatching timer event: ID=%d", args[0]);
                            result = wasm_runtime_call_wasm(node->ctx.exec_env, dispatcher, 1, args);
                            break;
                        case OCRE_RESOURCE_TYPE_GPIO:
                            args[0] = event->id;
                            args[1] = event->state;
                            LOG_INF("Dispatching GPIO event: pin=%d, state=%d", args[0], args[1]);
                            result = wasm_runtime_call_wasm(node->ctx.exec_env, dispatcher, 2, args);
                            break;
                        case OCRE_RESOURCE_TYPE_SENSOR:
                            args[0] = event->id;
                            args[1] = event->port;
                            args[2] = event->state;
                            LOG_INF("Dispatching sensor event: ID=%d, channel=%d, value=%d", args[0], args[1], args[2]);
                            result = wasm_runtime_call_wasm(node->ctx.exec_env, dispatcher, 3, args);
                            break;
                        default:
                            LOG_ERR("Unknown event type in dispatcher");
                            break;
                    }
                    if (!result) {
                        LOG_ERR("WASM call failed: %s", wasm_runtime_get_exception(module_inst));
                    }
                    current_module_tls = NULL;
                    node->ctx.last_activity = k_uptime_get_32();
                    break;
                }
            }
            k_mutex_unlock(&registry_mutex);
            if (!found) {
                LOG_ERR("Module instance %p not found in registry", (void *)module_inst);
            }
        }
    }

    // Clean up WASM runtime thread environment
    wasm_runtime_destroy_thread_env();
}

int ocre_get_event(wasm_exec_env_t exec_env, uint32_t type_offset, uint32_t id_offset, uint32_t port_offset,
                   uint32_t state_offset) {
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        LOG_ERR("No module instance for exec_env");
        return -EINVAL;
    }

    int32_t *type_native = (int32_t *)wasm_runtime_addr_app_to_native(module_inst, type_offset);
    int32_t *id_native = (int32_t *)wasm_runtime_addr_app_to_native(module_inst, id_offset);
    int32_t *port_native = (int32_t *)wasm_runtime_addr_app_to_native(module_inst, port_offset);
    int32_t *state_native = (int32_t *)wasm_runtime_addr_app_to_native(module_inst, state_offset);

    if (!type_native || !id_native || !port_native || !state_native) {
        LOG_ERR("Invalid offsets provided");
        return -EINVAL;
    }

    wasm_event_t event;
    k_spinlock_key_t key = k_spin_lock(&wasm_event_queue_lock);
    int ret = k_msgq_get(&wasm_event_queue, &event, K_NO_WAIT);
    if (ret != 0) {
        k_spin_unlock(&wasm_event_queue_lock, key);
        return -ENOENT;
    }

    *type_native = event.type;
    *id_native = event.id;
    *port_native = event.port;
    *state_native = event.state;

    LOG_INF("Retrieved event: type=%d, id=%d, port=%d, state=%d", event.type, event.id, event.port, event.state);
    k_spin_unlock(&wasm_event_queue_lock, key);
    return 0;
}

int ocre_common_init(void) {
    static bool initialized = false;
    if (initialized) {
        LOG_INF("Common system already initialized");
        return 0;
    }
    k_mutex_init(&registry_mutex);
    sys_slist_init(&module_registry);
    k_sem_init(&event_sem, 0, UINT_MAX);
    ring_buf_init(&event_ring, EVENT_BUFFER_SIZE, event_buffer);
    if ((uintptr_t)wasm_event_queue_buffer_ptr % 4 != 0) {
        LOG_ERR("wasm_event_queue_buffer misaligned: %p", (void *)wasm_event_queue_buffer_ptr);
        return -EINVAL;
    }
    k_msgq_init(&wasm_event_queue, wasm_event_queue_buffer, sizeof(wasm_event_t), 64);
    wasm_event_t dummy;
    while (k_msgq_get(&wasm_event_queue, &dummy, K_NO_WAIT) == 0) {
        LOG_INF("Purged stale event from queue");
    }
    wasm_event_queue_initialized = true;
    LOG_INF("wasm_event_queue initialized at %p, size=%d, buffer=%p", (void *)&wasm_event_queue, sizeof(wasm_event_t),
            (void *)wasm_event_queue_buffer_ptr);
    for (int i = 0; i < EVENT_THREAD_POOL_SIZE; i++) {
        event_args[i].index = i;
        char thread_name[16];
        snprintf(thread_name, sizeof(thread_name), "event_thread_%d", i);
        int ret = core_thread_create(&event_threads[i], event_thread_fn, &event_args[i], thread_name, EVENT_THREAD_STACK_SIZE, 5);
        if (ret != 0) {
            LOG_ERR("Failed to create thread for event %d", i);
            return -1;
        }
        LOG_INF("Started event thread %s", thread_name);
    }
    initialized = true;
    common_initialized = true;
    LOG_INF("OCRE common initialized successfully");
    return 0;
}

// Signal event threads to exit gracefully
void ocre_common_shutdown(void) {
    event_threads_exit = true;
    for (int i = 0; i < EVENT_THREAD_POOL_SIZE; i++) {
        k_sem_give(&event_sem);
    }
}

int ocre_register_cleanup_handler(ocre_resource_type_t type, ocre_cleanup_handler_t handler) {
    if (type >= OCRE_RESOURCE_TYPE_COUNT) {
        LOG_ERR("Invalid resource type: %d", type);
        return -EINVAL;
    }
    cleanup_handlers[type] = (struct cleanup_handler){type, handler};
    LOG_INF("Registered cleanup handler for type %d", type);
    return 0;
}

int ocre_register_module(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Null module instance");
        return -EINVAL;
    }
    module_node_t *node = k_malloc(sizeof(module_node_t));
    if (!node) {
        LOG_ERR("Failed to allocate module node");
        return -ENOMEM;
    }
    node->ctx.inst = module_inst;
    node->ctx.exec_env = wasm_runtime_create_exec_env(module_inst, OCRE_WASM_STACK_SIZE);
    if (!node->ctx.exec_env) {
        LOG_ERR("Failed to create exec env for module %p", (void *)module_inst);
        k_free(node);
        return -ENOMEM;
    }
    node->ctx.in_use = true;
    node->ctx.last_activity = k_uptime_get_32();
    memset(node->ctx.resource_count, 0, sizeof(node->ctx.resource_count));
    memset(node->ctx.dispatchers, 0, sizeof(node->ctx.dispatchers));
    k_mutex_lock(&registry_mutex, K_FOREVER);
    sys_slist_append(&module_registry, &node->node);
    k_mutex_unlock(&registry_mutex);
    LOG_INF("Module registered: %p", (void *)module_inst);
    return 0;
}

void ocre_unregister_module(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Null module instance");
        return;
    }
    k_mutex_lock(&registry_mutex, K_FOREVER);
    module_node_t *node, *tmp;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&module_registry, node, tmp, node) {
        if (node->ctx.inst == module_inst) {
            ocre_cleanup_module_resources(module_inst);
            if (node->ctx.exec_env) {
                wasm_runtime_destroy_exec_env(node->ctx.exec_env);
            }
            sys_slist_remove(&module_registry, NULL, &node->node);
            k_free(node);
            LOG_INF("Module unregistered: %p", (void *)module_inst);
            break;
        }
    }
    k_mutex_unlock(&registry_mutex);
}

ocre_module_context_t *ocre_get_module_context(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Null module instance");
        return NULL;
    }
    k_mutex_lock(&registry_mutex, K_FOREVER);
    module_node_t *node;
    SYS_SLIST_FOR_EACH_CONTAINER(&module_registry, node, node) {
        if (node->ctx.inst == module_inst) {
            node->ctx.last_activity = k_uptime_get_32();
            k_mutex_unlock(&registry_mutex);
            return &node->ctx;
        }
    }
    k_mutex_unlock(&registry_mutex);
    LOG_ERR("Module context not found for %p", (void *)module_inst);
    return NULL;
}

int ocre_register_dispatcher(wasm_exec_env_t exec_env, ocre_resource_type_t type, const char *function_name) {
    if (!exec_env || !function_name || type >= OCRE_RESOURCE_TYPE_COUNT) {
        LOG_ERR("Invalid dispatcher params: exec_env=%p, type=%d, func=%s", (void *)exec_env, type,
                function_name ? function_name : "null");
        return -EINVAL;
    }
    wasm_module_inst_t module_inst = current_module_tls ? *current_module_tls : wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        LOG_ERR("No module instance for event type %d", type);
        return -EINVAL;
    }
    ocre_module_context_t *ctx = ocre_get_module_context(module_inst);
    if (!ctx) {
        LOG_ERR("Module context not found for %p", (void *)module_inst);
        return -EINVAL;
    }
    LOG_DBG("Attempting to lookup function '%s' in module %p", function_name, (void *)module_inst);
    wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, function_name);
    if (!func) {
        LOG_ERR("Function %s not found in module %p", function_name, (void *)module_inst);
        return -EINVAL;
    }
    k_mutex_lock(&registry_mutex, K_FOREVER);
    ctx->dispatchers[type] = func;
    k_mutex_unlock(&registry_mutex);
    LOG_INF("Registered dispatcher for type %d: %s", type, function_name);
    return 0;
}

int ocre_post_event(ocre_event_t *event) {
    if (!event) {
        LOG_ERR("Null event");
        return -EINVAL;
    }
    if (event->type >= OCRE_RESOURCE_TYPE_COUNT) {
        LOG_ERR("Invalid event type: %d", event->type);
        return -EINVAL;
    }
    if (ring_buf_space_get(&event_ring) < sizeof(ocre_event_t)) {
        LOG_ERR("Event ring buffer full");
        return -ENOMEM;
    }
    if (ring_buf_put(&event_ring, (uint8_t *)event, sizeof(ocre_event_t)) != sizeof(ocre_event_t)) {
        LOG_ERR("Failed to post event to ring buffer");
        return -ENOMEM;
    }
    k_sem_give(&event_sem);
    LOG_INF("Posted event: type=%d", event->type);
    return 0;
}

uint32_t ocre_get_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type) {
    ocre_module_context_t *ctx = ocre_get_module_context(module_inst);
    return ctx ? ctx->resource_count[type] : 0;
}

void ocre_increment_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type) {
    ocre_module_context_t *ctx = ocre_get_module_context(module_inst);
    if (ctx && type < OCRE_RESOURCE_TYPE_COUNT) {
        k_mutex_lock(&registry_mutex, K_FOREVER);
        ctx->resource_count[type]++;
        k_mutex_unlock(&registry_mutex);
        LOG_INF("Incremented resource count: type=%d, count=%d", type, ctx->resource_count[type]);
    }
}

void ocre_decrement_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type) {
    ocre_module_context_t *ctx = ocre_get_module_context(module_inst);
    if (ctx && type < OCRE_RESOURCE_TYPE_COUNT && ctx->resource_count[type] > 0) {
        k_mutex_lock(&registry_mutex, K_FOREVER);
        ctx->resource_count[type]--;
        k_mutex_unlock(&registry_mutex);
        LOG_INF("Decremented resource count: type=%d, count=%d", type, ctx->resource_count[type]);
    }
}

void ocre_cleanup_module_resources(wasm_module_inst_t module_inst) {
    for (int i = 0; i < OCRE_RESOURCE_TYPE_COUNT; i++) {
        if (cleanup_handlers[i].handler) {
            cleanup_handlers[i].handler(module_inst);
        }
    }
}

wasm_module_inst_t ocre_get_current_module(void) {
    return current_module_tls ? *current_module_tls : NULL;
}
