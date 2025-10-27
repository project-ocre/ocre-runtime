/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre, a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include "ocre_core_external.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stddef.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#ifdef CONFIG_OCRE_GPIO
#include "../ocre_gpio/ocre_gpio.h"
#endif

#ifdef CONFIG_OCRE_CONTAINER_MESSAGING
#include "../ocre_messaging/ocre_messaging.h"
#endif

#include "ocre_common.h"

typedef struct module_node {
    ocre_module_context_t ctx;
    core_snode_t node;
} module_node_t;

static core_slist_t module_registry;
static core_mutex_t registry_mutex;

#define SIZE_OCRE_EVENT_BUFFER 32

/* Unified event queue implementation */
core_eventq_t ocre_event_queue;
bool ocre_event_queue_initialized = false;
core_spinlock_t ocre_event_queue_lock;

static struct cleanup_handler {
    ocre_resource_type_t type;
    ocre_cleanup_handler_t handler;
} cleanup_handlers[OCRE_RESOURCE_TYPE_COUNT];

bool common_initialized = false;

/* Thread-Local Storage */
__thread wasm_module_inst_t *current_module_tls = NULL;

#if EVENT_THREAD_POOL_SIZE > 0
// Arguments for event threads
struct event_thread_args {
    int index; // Thread index for identification or logging
};
static struct core_thread event_threads[EVENT_THREAD_POOL_SIZE];
// Flag to signal event threads to exit gracefully
static volatile bool event_threads_exit = false;
static struct event_thread_args event_args[EVENT_THREAD_POOL_SIZE];

// Event thread function to process events from the ring buffer
static void event_thread_fn(void *arg1) {
    struct event_thread_args *args = (struct event_thread_args *)arg1;
    // Initialize WASM runtime thread environment
    wasm_runtime_init_thread_env();
    // Main event processing loop
    // Do something in wasm
    // Saved for future usage
    // Clean up WASM runtime thread envirnment
    wasm_runtime_destroy_thread_env();
}
#endif

int ocre_get_event(wasm_exec_env_t exec_env, uint32_t type_offset, uint32_t id_offset, uint32_t port_offset,
                   uint32_t state_offset, uint32_t extra_offset, uint32_t payload_len_offset) {
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        LOG_ERR("No module instance for exec_env\n");
        return -EINVAL;
    }

    uint32_t *type_native = (uint32_t *)wasm_runtime_addr_app_to_native(module_inst, type_offset);
    uint32_t *id_native = (uint32_t *)wasm_runtime_addr_app_to_native(module_inst, id_offset);
    uint32_t *port_native = (uint32_t *)wasm_runtime_addr_app_to_native(module_inst, port_offset);
    uint32_t *state_native = (uint32_t *)wasm_runtime_addr_app_to_native(module_inst, state_offset);
    uint32_t *extra_native = (uint32_t *)wasm_runtime_addr_app_to_native(module_inst, extra_offset);
    uint32_t *payload_len_native = (uint32_t *)wasm_runtime_addr_app_to_native(module_inst, payload_len_offset);

    if (!type_native || !id_native || !port_native || !state_native || !extra_native || !payload_len_native) {
        LOG_ERR("Invalid offsets provided");
        return -EINVAL;
    }

    ocre_event_t event;
    int ret;
    
    /* Generic event queue implementation for both platforms */
    core_spinlock_key_t key = core_spinlock_lock(&ocre_event_queue_lock);
    ret = core_eventq_peek(&ocre_event_queue, &event);
    if (ret != 0) {
        core_spinlock_unlock(&ocre_event_queue_lock, key);
        return -ENOMSG;
    }
    
    if (event.owner != module_inst) {
        core_spinlock_unlock(&ocre_event_queue_lock, key);
        return -EPERM;
    }

    ret = core_eventq_get(&ocre_event_queue, &event);
    if (ret != 0) {
        core_spinlock_unlock(&ocre_event_queue_lock, key);
        return -ENOENT;
    }

    // Send event correctly to WASM
    switch (event.type) {
        case OCRE_RESOURCE_TYPE_TIMER: {
            LOG_DBG("Retrieved Timer event timer_id=%u, owner=%p", event.data.timer_event.timer_id,
                    (void *)event.owner);
            *type_native = event.type;
            *id_native = event.data.timer_event.timer_id;
            *port_native = 0;
            *state_native = 0;
            *extra_native = 0;
            *payload_len_native = 0;
            break;
        }
        case OCRE_RESOURCE_TYPE_GPIO: {
            LOG_DBG("Retrieved Gpio event pin_id=%u, port=%u, state=%u, owner=%p", event.data.gpio_event.pin_id,
                    event.data.gpio_event.port, event.data.gpio_event.state, (void *)event.owner);
            *type_native = event.type;
            *id_native = event.data.gpio_event.pin_id;
            *port_native = event.data.gpio_event.port;
            *state_native = event.data.gpio_event.state;
            *extra_native = 0;
            *payload_len_native = 0;
            break;
        }
        case OCRE_RESOURCE_TYPE_SENSOR: {
            // Not used as we don't use callbacks in sensor API yet
            break;
        }
        case OCRE_RESOURCE_TYPE_MESSAGING: {
            LOG_DBG("Retrieved Messaging event: message_id=%u, topic=%s, topic_offset=%u, content_type=%s, "
                    "content_type_offset=%u, payload_len=%d, owner=%p",
                    event.data.messaging_event.message_id, event.data.messaging_event.topic,
                    event.data.messaging_event.topic_offset, event.data.messaging_event.content_type,
                    event.data.messaging_event.content_type_offset, event.data.messaging_event.payload_len,
                    (void *)event.owner);
            *type_native = event.type;
            *id_native = event.data.messaging_event.message_id;
            *port_native = event.data.messaging_event.topic_offset;
            *state_native = event.data.messaging_event.content_type_offset;
            *extra_native = event.data.messaging_event.payload_offset;
            *payload_len_native = event.data.messaging_event.payload_len;
            break;
        }
        /*
            =================================
            Place to add more resource types
            =================================
        */
        default: {
            core_spinlock_unlock(&ocre_event_queue_lock, key);
            LOG_ERR("Invalid event type: %u", event.type);
            return -EINVAL;
        }
    }
    core_spinlock_unlock(&ocre_event_queue_lock, key);
    return 0;
}

int ocre_common_init(void) {
    static bool initialized = false;
    if (initialized) {
        LOG_INF("Common system already initialized");
        return 0;
    }
    
    core_mutex_init(&registry_mutex);
    core_slist_init(&module_registry);
    
    core_eventq_init(&ocre_event_queue, sizeof(ocre_event_t), SIZE_OCRE_EVENT_BUFFER);
    
    /* Purge any stale events from queue */
    ocre_event_t dummy;
    while (core_eventq_get(&ocre_event_queue, &dummy) == 0) {
        LOG_INF("Purged stale event from queue");
    }

    /* Temporary platform-specific initialization */
#ifdef __ZEPHYR__
    /* No additional Zephyr-specific initialization needed */
#else /* POSIX */
    pthread_mutex_init(&ocre_event_queue_lock.mutex, NULL);
#endif

    ocre_event_queue_initialized = true;
#if EVENT_THREAD_POOL_SIZE > 0
    LOG_INF("ocre_event_queue initialized at %p, size=%d", (void *)&ocre_event_queue, sizeof(ocre_event_t));
    for (int i = 0; i < EVENT_THREAD_POOL_SIZE; i++) {
        event_args[i].index = i;
        char thread_name[16];
        snprintf(thread_name, sizeof(thread_name), "event_thread_%d", i);
        int ret = core_thread_create(&event_threads[i], event_thread_fn, &event_args[i], thread_name,
                                     EVENT_THREAD_STACK_SIZE, 5);
        if (ret != 0) {
            LOG_ERR("Failed to create thread for event %d", i);
            return -1;
        }
        LOG_INF("Started event thread %s", thread_name);
    }
#endif
    initialized = true;
    common_initialized = true;
            
    LOG_INF("OCRE common initialized successfully");
    return 0;
}

void ocre_common_shutdown(void) {
#if EVENT_THREAD_POOL_SIZE > 0
    event_threads_exit = true;
    for (int i = EVENT_THREAD_POOL_SIZE; i > 0; i--) {
        event_args[i].index = i;
        char thread_name[16];
        snprintf(thread_name, sizeof(thread_name), "event_thread_%d", i);
        core_thread_destroy(&event_threads[i]);
    }
#endif
    common_initialized = false;
    LOG_INF("OCRE common shutdown successfully");
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
    module_node_t *node = core_malloc(sizeof(module_node_t));
    if (!node) {
        LOG_ERR("Failed to allocate module node");
        return -ENOMEM;
    }
    node->ctx.inst = module_inst;
    node->ctx.exec_env = wasm_runtime_create_exec_env(module_inst, OCRE_WASM_STACK_SIZE);
    if (!node->ctx.exec_env) {
        LOG_ERR("Failed to create exec env for module %p", (void *)module_inst);
        core_free(node);
        return -ENOMEM;
    }
    node->ctx.in_use = true;
    node->ctx.last_activity = core_uptime_get();
    memset(node->ctx.resource_count, 0, sizeof(node->ctx.resource_count));
    memset(node->ctx.dispatchers, 0, sizeof(node->ctx.dispatchers));
    
    core_mutex_lock(&registry_mutex);
    core_slist_append(&module_registry, &node->node);
    core_mutex_unlock(&registry_mutex);
    
    LOG_INF("Module registered: %p", (void *)module_inst);
    return 0;
}

void ocre_unregister_module(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Null module instance");
        return;
    }
    
    core_mutex_lock(&registry_mutex);
    module_node_t *node, *tmp;
    module_node_t *prev = NULL;
    CORE_SLIST_FOR_EACH_CONTAINER_SAFE(&module_registry, node, tmp, node) {
        if (node->ctx.inst == module_inst) {
            ocre_cleanup_module_resources(module_inst);
            if (node->ctx.exec_env) {
                wasm_runtime_destroy_exec_env(node->ctx.exec_env);
            }
            core_slist_remove(&module_registry, prev ? &prev->node : NULL, &node->node);
            core_free(node);
            LOG_INF("Module unregistered: %p", (void *)module_inst);
            break;
        }
        prev = node;
    }
    core_mutex_unlock(&registry_mutex);
}

ocre_module_context_t *ocre_get_module_context(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Null module instance");
        return NULL;
    }
    core_mutex_lock(&registry_mutex);
    for (core_snode_t *current = module_registry.head; current != NULL; current = current->next) {
        module_node_t *node = (module_node_t *)((char *)current - offsetof(module_node_t, node));
        if (node->ctx.inst == module_inst) {
            node->ctx.last_activity = core_uptime_get();
            core_mutex_unlock(&registry_mutex);
            return &node->ctx;
        }
    }
    core_mutex_unlock(&registry_mutex);
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
    core_mutex_lock(&registry_mutex);
    ctx->dispatchers[type] = func;
    core_mutex_unlock(&registry_mutex);
    LOG_INF("Registered dispatcher for type %d: %s", type, function_name);
    return 0;
}

uint32_t ocre_get_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type) {
    ocre_module_context_t *ctx = ocre_get_module_context(module_inst);
    return ctx ? ctx->resource_count[type] : 0;
}

void ocre_increment_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type) {
    ocre_module_context_t *ctx = ocre_get_module_context(module_inst);
    if (ctx && type < OCRE_RESOURCE_TYPE_COUNT) {
        core_mutex_lock(&registry_mutex);
        ctx->resource_count[type]++;
        core_mutex_unlock(&registry_mutex);
        LOG_INF("Incremented resource count: type=%d, count=%d", type, ctx->resource_count[type]);
    }
}

void ocre_decrement_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type) {
    ocre_module_context_t *ctx = ocre_get_module_context(module_inst);
    if (ctx && type < OCRE_RESOURCE_TYPE_COUNT && ctx->resource_count[type] > 0) {
        core_mutex_lock(&registry_mutex);
        ctx->resource_count[type]--;
        core_mutex_unlock(&registry_mutex);
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
