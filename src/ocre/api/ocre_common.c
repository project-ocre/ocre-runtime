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

#ifdef CONFIG_ZEPHYR
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/mem_manage.h>
#include <zephyr/drivers/mm/system_mm.h>
#include <zephyr/sys/slist.h>
LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);
#include <zephyr/autoconf.h>
#include <zephyr/sys/ring_buffer.h>
#else /* POSIX */
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
/* POSIX logging macros are already defined in core_internal.h */
#endif

#include "../../../../../wasm-micro-runtime/core/iwasm/include/lib_export.h"
#include "bh_log.h"

#ifdef CONFIG_ZEPHYR
#include "../ocre_gpio/ocre_gpio.h"
#include "../ocre_messaging/ocre_messaging.h"
#endif

#include "ocre_common.h"

/* Platform-specific abstractions */
#ifdef CONFIG_ZEPHYR

#define SIZE_OCRE_EVENT_BUFFER 32
__attribute__((section(".noinit.ocre_event_queue"),
               aligned(8))) char ocre_event_queue_buffer[SIZE_OCRE_EVENT_BUFFER * sizeof(ocre_event_t)];
char *ocre_event_queue_buffer_ptr = ocre_event_queue_buffer;

K_MSGQ_DEFINE(ocre_event_queue, sizeof(ocre_event_t), SIZE_OCRE_EVENT_BUFFER, 4);
bool ocre_event_queue_initialized = false;
struct k_spinlock ocre_event_queue_lock;

typedef struct module_node {
    ocre_module_context_t ctx;
    sys_snode_t node;
} module_node_t;

static sys_slist_t module_registry;
static struct k_mutex registry_mutex;

/* Zephyr-specific macros */
#define OCRE_MALLOC(size)           k_malloc(size)
#define OCRE_FREE(ptr)              k_free(ptr)
#define OCRE_UPTIME_GET()           k_uptime_get_32()
#define OCRE_MUTEX_LOCK(mutex)      k_mutex_lock(mutex, K_FOREVER)
#define OCRE_MUTEX_UNLOCK(mutex)    k_mutex_unlock(mutex)
#define OCRE_MUTEX_INIT(mutex)      k_mutex_init(mutex)
#define OCRE_SPINLOCK_LOCK(lock)    k_spin_lock(lock)
#define OCRE_SPINLOCK_UNLOCK(lock, key) k_spin_unlock(lock, key)

#else /* POSIX */

#define SIZE_OCRE_EVENT_BUFFER 32

/* POSIX simple linked list implementation */
typedef struct posix_snode {
    struct posix_snode *next;
} posix_snode_t;

typedef struct {
    posix_snode_t *head;
    posix_snode_t *tail;
} posix_slist_t;

/* POSIX message queue simulation */
typedef struct {
    ocre_event_t *buffer;
    size_t size;
    size_t count;
    size_t head;
    size_t tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} posix_msgq_t;

/* POSIX spinlock simulation using mutex */
typedef struct {
    pthread_mutex_t mutex;
} posix_spinlock_t;

typedef int posix_spinlock_key_t;

static char ocre_event_queue_buffer[SIZE_OCRE_EVENT_BUFFER * sizeof(ocre_event_t)];
char *ocre_event_queue_buffer_ptr = ocre_event_queue_buffer;

static posix_msgq_t ocre_event_queue;
bool ocre_event_queue_initialized = false;
static posix_spinlock_t ocre_event_queue_lock;

typedef struct module_node {
    ocre_module_context_t ctx;
    posix_snode_t node;
} module_node_t;

static posix_slist_t module_registry;
static pthread_mutex_t registry_mutex;

/* POSIX-specific macros */
#define OCRE_MALLOC(size)           malloc(size)
#define OCRE_FREE(ptr)              free(ptr)
#define OCRE_UPTIME_GET()           posix_uptime_get()
#define OCRE_MUTEX_LOCK(mutex)      pthread_mutex_lock(mutex)
#define OCRE_MUTEX_UNLOCK(mutex)    pthread_mutex_unlock(mutex)
#define OCRE_MUTEX_INIT(mutex)      pthread_mutex_init(mutex, NULL)
#define OCRE_SPINLOCK_LOCK(lock)    posix_spinlock_lock(lock)
#define OCRE_SPINLOCK_UNLOCK(lock, key) posix_spinlock_unlock(lock, key)

/* POSIX error codes compatibility */
#ifndef EINVAL
#define EINVAL          22
#endif
#ifndef ENOMSG
#define ENOMSG          42
#endif
#ifndef EPERM
#define EPERM           1
#endif
#ifndef ENOENT
#define ENOENT          2
#endif
#ifndef ENOMEM
#define ENOMEM          12
#endif

/* POSIX helper functions */
static uint32_t posix_uptime_get(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static posix_spinlock_key_t posix_spinlock_lock(posix_spinlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    return 0;
}

static void posix_spinlock_unlock(posix_spinlock_t *lock, posix_spinlock_key_t key) {
    (void)key;
    pthread_mutex_unlock(&lock->mutex);
}

static void posix_slist_init(posix_slist_t *list) {
    list->head = NULL;
    list->tail = NULL;
}

static void posix_slist_append(posix_slist_t *list, posix_snode_t *node) {
    node->next = NULL;
    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
}

static void posix_slist_remove(posix_slist_t *list, posix_snode_t *prev, posix_snode_t *node) {
    if (prev) {
        prev->next = node->next;
    } else {
        list->head = node->next;
    }
    if (list->tail == node) {
        list->tail = prev;
    }
}

static int posix_msgq_init(posix_msgq_t *msgq, size_t item_size, size_t max_items) {
    msgq->buffer = (ocre_event_t *)ocre_event_queue_buffer;
    msgq->size = max_items;
    msgq->count = 0;
    msgq->head = 0;
    msgq->tail = 0;
    pthread_mutex_init(&msgq->mutex, NULL);
    pthread_cond_init(&msgq->cond, NULL);
    return 0;
}

static int posix_msgq_peek(posix_msgq_t *msgq, ocre_event_t *event) {
    if (msgq->count == 0) {
        return -ENOMSG;
    }
    *event = msgq->buffer[msgq->head];
    return 0;
}

static int posix_msgq_get(posix_msgq_t *msgq, ocre_event_t *event) {
    if (msgq->count == 0) {
        return -ENOENT;
    }
    *event = msgq->buffer[msgq->head];
    msgq->head = (msgq->head + 1) % msgq->size;
    msgq->count--;
    return 0;
}

static int posix_msgq_put(posix_msgq_t *msgq, const ocre_event_t *event) {
    if (msgq->count >= msgq->size) {
        return -ENOMEM;
    }
    msgq->buffer[msgq->tail] = *event;
    msgq->tail = (msgq->tail + 1) % msgq->size;
    msgq->count++;
    return 0;
}

#define SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, var, tmp, member) \
    for (var = (module_node_t *)((list)->head), \
         tmp = var ? (module_node_t *)(var->node.next) : NULL; \
         var; \
         var = tmp, tmp = tmp ? (module_node_t *)(tmp->node.next) : NULL)

#define SYS_SLIST_FOR_EACH_CONTAINER(list, var, member) \
    for (var = (module_node_t *)((list)->head); \
         var; \
         var = (module_node_t *)(var->node.next))

#endif /* CONFIG_ZEPHYR */

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
    
#ifdef CONFIG_ZEPHYR
    k_spinlock_key_t key = k_spin_lock(&ocre_event_queue_lock);
    ret = k_msgq_peek(&ocre_event_queue, &event);
    if (ret != 0) {
        k_spin_unlock(&ocre_event_queue_lock, key);
        return -ENOMSG;
    }
    
    if (event.owner != module_inst) {
        k_spin_unlock(&ocre_event_queue_lock, key);
        return -EPERM;
    }

    ret = k_msgq_get(&ocre_event_queue, &event, K_FOREVER);
    if (ret != 0) {
        k_spin_unlock(&ocre_event_queue_lock, key);
        return -ENOENT;
    }
#else /* POSIX */
    posix_spinlock_key_t key = OCRE_SPINLOCK_LOCK(&ocre_event_queue_lock);
    ret = posix_msgq_peek(&ocre_event_queue, &event);
    if (ret != 0) {
        OCRE_SPINLOCK_UNLOCK(&ocre_event_queue_lock, key);
        return -ENOMSG;
    }
    
    if (event.owner != module_inst) {
        OCRE_SPINLOCK_UNLOCK(&ocre_event_queue_lock, key);
        return -EPERM;
    }

    ret = posix_msgq_get(&ocre_event_queue, &event);
    if (ret != 0) {
        OCRE_SPINLOCK_UNLOCK(&ocre_event_queue_lock, key);
        return -ENOENT;
    }
#endif

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
#ifdef CONFIG_ZEPHYR
            k_spin_unlock(&ocre_event_queue_lock, key);
#else
            OCRE_SPINLOCK_UNLOCK(&ocre_event_queue_lock, key);
#endif
            LOG_ERR("Invalid event type: %u", event.type);
            return -EINVAL;
        }
    }
#ifdef CONFIG_ZEPHYR
    k_spin_unlock(&ocre_event_queue_lock, key);
#else
    OCRE_SPINLOCK_UNLOCK(&ocre_event_queue_lock, key);
#endif
    return 0;
}

int ocre_common_init(void) {
    static bool initialized = false;
    if (initialized) {
        LOG_INF("Common system already initialized");
        return 0;
    }
    
#ifdef CONFIG_ZEPHYR
    k_mutex_init(&registry_mutex);
    sys_slist_init(&module_registry);
    if ((uintptr_t)ocre_event_queue_buffer_ptr % 4 != 0) {
        LOG_ERR("ocre_event_queue_buffer misaligned: %p", (void *)ocre_event_queue_buffer_ptr);
        return -EINVAL;
    }
    k_msgq_init(&ocre_event_queue, ocre_event_queue_buffer, sizeof(ocre_event_t), 64);
    ocre_event_t dummy;
    while (k_msgq_get(&ocre_event_queue, &dummy, K_NO_WAIT) == 0) {
        LOG_INF("Purged stale event from queue");
    }
#else /* POSIX */
    OCRE_MUTEX_INIT(&registry_mutex);
    posix_slist_init(&module_registry);
    if ((uintptr_t)ocre_event_queue_buffer_ptr % 4 != 0) {
        LOG_ERR("ocre_event_queue_buffer misaligned: %p", (void *)ocre_event_queue_buffer_ptr);
        return -EINVAL;
    }
    posix_msgq_init(&ocre_event_queue, sizeof(ocre_event_t), SIZE_OCRE_EVENT_BUFFER);
    pthread_mutex_init(&ocre_event_queue_lock.mutex, NULL);
#endif
    
    ocre_event_queue_initialized = true;
#if EVENT_THREAD_POOL_SIZE > 0
    LOG_INF("ocre_event_queue initialized at %p, size=%d, buffer=%p", (void *)&ocre_event_queue, sizeof(ocre_event_t),
            (void *)ocre_event_queue_buffer_ptr);
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
    
    /* Initialize timer system as part of common initialization */
    ocre_timer_init();
    
    /* Initialize messaging system as part of common initialization */
    ocre_messaging_init();
    
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
    module_node_t *node = OCRE_MALLOC(sizeof(module_node_t));
    if (!node) {
        LOG_ERR("Failed to allocate module node");
        return -ENOMEM;
    }
    node->ctx.inst = module_inst;
    node->ctx.exec_env = wasm_runtime_create_exec_env(module_inst, OCRE_WASM_STACK_SIZE);
    if (!node->ctx.exec_env) {
        LOG_ERR("Failed to create exec env for module %p", (void *)module_inst);
        OCRE_FREE(node);
        return -ENOMEM;
    }
    node->ctx.in_use = true;
    node->ctx.last_activity = OCRE_UPTIME_GET();
    memset(node->ctx.resource_count, 0, sizeof(node->ctx.resource_count));
    memset(node->ctx.dispatchers, 0, sizeof(node->ctx.dispatchers));
    
#ifdef CONFIG_ZEPHYR
    k_mutex_lock(&registry_mutex, K_FOREVER);
    sys_slist_append(&module_registry, &node->node);
    k_mutex_unlock(&registry_mutex);
#else
    OCRE_MUTEX_LOCK(&registry_mutex);
    posix_slist_append(&module_registry, &node->node);
    OCRE_MUTEX_UNLOCK(&registry_mutex);
#endif
    
    LOG_INF("Module registered: %p", (void *)module_inst);
    return 0;
}

void ocre_unregister_module(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Null module instance");
        return;
    }
    
#ifdef CONFIG_ZEPHYR
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
#else
    OCRE_MUTEX_LOCK(&registry_mutex);
    module_node_t *node, *tmp;
    module_node_t *prev = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&module_registry, node, tmp, node) {
        if (node->ctx.inst == module_inst) {
            ocre_cleanup_module_resources(module_inst);
            if (node->ctx.exec_env) {
                wasm_runtime_destroy_exec_env(node->ctx.exec_env);
            }
            posix_slist_remove(&module_registry, prev ? &prev->node : NULL, &node->node);
            OCRE_FREE(node);
            LOG_INF("Module unregistered: %p", (void *)module_inst);
            break;
        }
        prev = node;
    }
    OCRE_MUTEX_UNLOCK(&registry_mutex);
#endif
}

ocre_module_context_t *ocre_get_module_context(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Null module instance");
        return NULL;
    }
    
    LOG_DBG("Looking for module context: %p", (void *)module_inst);
    
    int count = 0;
    
#ifdef CONFIG_ZEPHYR
    k_mutex_lock(&registry_mutex, K_FOREVER);
    for (sys_snode_t *current = module_registry.head; current != NULL; current = current->next) {
        module_node_t *node = (module_node_t *)((char *)current - offsetof(module_node_t, node));
        LOG_DBG("  Registry entry %d: %p", count++, (void *)node->ctx.inst);
        if (node->ctx.inst == module_inst) {
            node->ctx.last_activity = k_uptime_get_32();
            k_mutex_unlock(&registry_mutex);
            LOG_DBG("Found module context for %p", (void *)module_inst);
            return &node->ctx;
        }
    }
    k_mutex_unlock(&registry_mutex);
#else
    OCRE_MUTEX_LOCK(&registry_mutex);
    for (posix_snode_t *current = module_registry.head; current != NULL; current = current->next) {
        module_node_t *node = (module_node_t *)((char *)current - offsetof(module_node_t, node));
        LOG_DBG("  Registry entry %d: %p", count++, (void *)node->ctx.inst);
        if (node->ctx.inst == module_inst) {
            node->ctx.last_activity = OCRE_UPTIME_GET();
            OCRE_MUTEX_UNLOCK(&registry_mutex);
            LOG_DBG("Found module context for %p", (void *)module_inst);
            return &node->ctx;
        }
    }
    OCRE_MUTEX_UNLOCK(&registry_mutex);
#endif
    
    LOG_ERR("Module context not found for %p", (void *)module_inst);
    return NULL;
}

int ocre_register_dispatcher(wasm_exec_env_t exec_env, ocre_resource_type_t type, const char *function_name) {
    if (!exec_env || !function_name || type >= OCRE_RESOURCE_TYPE_COUNT) {
        LOG_ERR("Invalid dispatcher params: exec_env=%p, type=%d, func=%s", (void *)exec_env, type,
                function_name ? function_name : "null");
        return -EINVAL;
    }
    
    LOG_DBG("ocre_register_dispatcher: exec_env=%p, type=%d, func=%s", (void *)exec_env, type, function_name);
    LOG_DBG("current_module_tls = %p", current_module_tls ? (void *)*current_module_tls : NULL);
    
    wasm_module_inst_t module_inst = current_module_tls ? *current_module_tls : wasm_runtime_get_module_inst(exec_env);
    LOG_DBG("Retrieved module_inst = %p", (void *)module_inst);
    
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
#ifdef CONFIG_ZEPHYR
    k_mutex_lock(&registry_mutex, K_FOREVER);
#else
    OCRE_MUTEX_LOCK(&registry_mutex);
#endif
    ctx->dispatchers[type] = func;
#ifdef CONFIG_ZEPHYR
    k_mutex_unlock(&registry_mutex);
#else
    OCRE_MUTEX_UNLOCK(&registry_mutex);
#endif
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
#ifdef CONFIG_ZEPHYR
        k_mutex_lock(&registry_mutex, K_FOREVER);
#else
        OCRE_MUTEX_LOCK(&registry_mutex);
#endif
        ctx->resource_count[type]++;
#ifdef CONFIG_ZEPHYR
        k_mutex_unlock(&registry_mutex);
#else
        OCRE_MUTEX_UNLOCK(&registry_mutex);
#endif
        LOG_INF("Incremented resource count: type=%d, count=%d", type, ctx->resource_count[type]);
    }
}

void ocre_decrement_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type) {
    ocre_module_context_t *ctx = ocre_get_module_context(module_inst);
    if (ctx && type < OCRE_RESOURCE_TYPE_COUNT && ctx->resource_count[type] > 0) {
#ifdef CONFIG_ZEPHYR
        k_mutex_lock(&registry_mutex, K_FOREVER);
#else
        OCRE_MUTEX_LOCK(&registry_mutex);
#endif
        ctx->resource_count[type]--;
#ifdef CONFIG_ZEPHYR
        k_mutex_unlock(&registry_mutex);
#else
        OCRE_MUTEX_UNLOCK(&registry_mutex);
#endif
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

/* ========== OCRE TIMER FUNCTIONALITY ========== */

#include "ocre_common.h"

/* Timer type definition */
typedef uint32_t ocre_timer_t;

/* Platform-specific timer structures */
#ifdef CONFIG_ZEPHYR
// Compact timer structure for Zephyr
typedef struct {
    uint32_t in_use: 1;
    uint32_t id: 8;        // Up to 256 timers
    uint32_t interval: 16; // Up to 65s intervals
    uint32_t periodic: 1;
    struct k_timer timer; 
    wasm_module_inst_t owner;
} ocre_timer_internal;

#else /* POSIX */
// POSIX timer structure
typedef struct {
    uint32_t in_use: 1;
    uint32_t id: 8;        // Up to 256 timers
    uint32_t interval: 16; // Up to 65s intervals
    uint32_t periodic: 1;
    timer_t timer;
    pthread_t thread;
    bool thread_running;
    wasm_module_inst_t owner;
} ocre_timer_internal;

/* POSIX timer thread function - not used currently */
#endif

#ifndef CONFIG_MAX_TIMER
#define CONFIG_MAX_TIMERS 5
#endif

// Static data
static ocre_timer_internal timers[CONFIG_MAX_TIMERS];
static bool timer_system_initialized = false;

#ifdef CONFIG_ZEPHYR
static void timer_callback_wrapper(struct k_timer *timer);
#else
static void posix_timer_signal_handler(int sig, siginfo_t *si, void *uc);
static void posix_send_timer_event(int timer_id);
#endif

void ocre_timer_init(void) {
    if (timer_system_initialized) {
        LOG_INF("Timer system already initialized");
        return;
    }

    if (!common_initialized && ocre_common_init() != 0) {
        LOG_ERR("Failed to initialize common subsystem");
        return;
    }

#ifndef CONFIG_ZEPHYR
    // Setup POSIX signal handler for timer callbacks
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = posix_timer_signal_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        LOG_ERR("Failed to setup POSIX timer signal handler");
        return;
    }
#endif

    ocre_register_cleanup_handler(OCRE_RESOURCE_TYPE_TIMER, ocre_timer_cleanup_container);
    timer_system_initialized = true;
    LOG_INF("Timer system initialized");
}

int ocre_timer_create(wasm_exec_env_t exec_env, int id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d (max: %d)", (void *)module, id, CONFIG_MAX_TIMERS);
        return -EINVAL;
    }

    ocre_timer_internal *timer = &timers[id - 1];
    if (timer->in_use) {
        LOG_ERR("Timer ID %d already in use", id);
        return -EBUSY;
    }

    timer->id = id;
    timer->owner = module;
    timer->in_use = 1;
    
#ifdef CONFIG_ZEPHYR
    k_timer_init(&timer->timer, timer_callback_wrapper, NULL);
#else
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_int = id;
    
    if (timer_create(CLOCK_REALTIME, &sev, &timer->timer) == -1) {
        LOG_ERR("Failed to create POSIX timer %d", id);
        timer->in_use = 0;
        return -EINVAL;
    }
    timer->thread_running = false;
#endif
    
    ocre_increment_resource_count(module, OCRE_RESOURCE_TYPE_TIMER);
    LOG_INF("Created timer %d for module %p", id, (void *)module);
    return 0;
}

int ocre_timer_delete(wasm_exec_env_t exec_env, ocre_timer_t id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer_internal *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }
    
#ifdef CONFIG_ZEPHYR
    k_timer_stop(&timer->timer);
#else
    timer_delete(&timer->timer);
    if (timer->thread_running) {
        pthread_cancel(timer->thread);
        timer->thread_running = false;
    }
#endif
    
    timer->in_use = 0;
    timer->owner = NULL;
    ocre_decrement_resource_count(module, OCRE_RESOURCE_TYPE_TIMER);
    LOG_INF("Deleted timer %d", id);
    return 0;
}

int ocre_timer_start(wasm_exec_env_t exec_env, ocre_timer_t id, int interval, int is_periodic) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer_internal *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }

    if (interval <= 0 || interval > 65535) {
        LOG_ERR("Invalid interval %dms (must be 1-65535ms)", interval);
        return -EINVAL;
    }

    timer->interval = interval;
    timer->periodic = is_periodic;
    
#ifdef CONFIG_ZEPHYR
    k_timeout_t duration = K_MSEC(interval);
    k_timeout_t period = is_periodic ? duration : K_NO_WAIT;
    k_timer_start(&timer->timer, duration, period);
#else
    struct itimerspec its;
    its.it_value.tv_sec = interval / 1000;
    its.it_value.tv_nsec = (interval % 1000) * 1000000;
    
    if (is_periodic) {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    } else {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
    }
    
    if (timer_settime(timer->timer, 0, &its, NULL) == -1) {
        LOG_ERR("Failed to start POSIX timer %d", id);
        return -EINVAL;
    }
#endif
    
    LOG_INF("Started timer %d with interval %dms, periodic=%d", id, interval, is_periodic);
    return 0;
}

int ocre_timer_stop(wasm_exec_env_t exec_env, ocre_timer_t id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer_internal *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }
    
#ifdef CONFIG_ZEPHYR
    k_timer_stop(&timer->timer);
#else
    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    timer_settime(timer->timer, 0, &its, NULL);
#endif
    
    LOG_INF("Stopped timer %d", id);
    return 0;
}

int ocre_timer_get_remaining(wasm_exec_env_t exec_env, ocre_timer_t id) {
    wasm_module_inst_t module = wasm_runtime_get_module_inst(exec_env);
    if (!module || id <= 0 || id > CONFIG_MAX_TIMERS) {
        LOG_ERR("Invalid module %p or timer ID %d", (void *)module, id);
        return -EINVAL;
    }

    ocre_timer_internal *timer = &timers[id - 1];
    if (!timer->in_use || timer->owner != module) {
        LOG_ERR("Timer ID %d not in use or not owned by module %p", id, (void *)module);
        return -EINVAL;
    }
    
    int remaining;
#ifdef CONFIG_ZEPHYR
    remaining = k_ticks_to_ms_floor32(k_timer_remaining_ticks(&timer->timer));
#else
    struct itimerspec its;
    if (timer_gettime(timer->timer, &its) == -1) {
        LOG_ERR("Failed to get remaining time for timer %d", id);
        return -EINVAL;
    }
    remaining = its.it_value.tv_sec * 1000 + its.it_value.tv_nsec / 1000000;
#endif
    
    LOG_INF("Timer %d remaining time: %dms", id, remaining);
    return remaining;
}

void ocre_timer_cleanup_container(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Invalid module instance for cleanup");
        return;
    }

    for (int i = 0; i < CONFIG_MAX_TIMERS; i++) {
        if (timers[i].in_use && timers[i].owner == module_inst) {
#ifdef CONFIG_ZEPHYR
            k_timer_stop(&timers[i].timer);
#else
            timer_delete(&timers[i].timer);
            if (timers[i].thread_running) {
                pthread_cancel(timers[i].thread);
                timers[i].thread_running = false;
            }
#endif
            timers[i].in_use = 0;
            timers[i].owner = NULL;
            ocre_decrement_resource_count(module_inst, OCRE_RESOURCE_TYPE_TIMER);
            LOG_INF("Cleaned up timer %d for module %p", i + 1, (void *)module_inst);
        }
    }
    LOG_INF("Cleaned up timer resources for module %p", (void *)module_inst);
}

#ifdef CONFIG_ZEPHYR
static void timer_callback_wrapper(struct k_timer *timer) {
    if (!timer_system_initialized || !common_initialized || !ocre_event_queue_initialized) {
        LOG_ERR("Timer, common, or event queue not initialized, skipping callback");
        return;
    }
    if (!timer) {
        LOG_ERR("Null timer pointer in callback");
        return;
    }
    if ((uintptr_t)ocre_event_queue_buffer_ptr % 4 != 0) {
        LOG_ERR("ocre_event_queue_buffer misaligned: %p", (void *)ocre_event_queue_buffer_ptr);
        return;
    }
    LOG_DBG("Timer callback for timer %p", (void *)timer);
    LOG_DBG("ocre_event_queue at %p, buffer at %p", (void *)&ocre_event_queue, (void *)ocre_event_queue_buffer_ptr);
    for (int i = 0; i < CONFIG_MAX_TIMERS; i++) {
        if (timers[i].in_use && &timers[i].timer == timer && timers[i].owner) {
            ocre_event_t event;
            event.type = OCRE_RESOURCE_TYPE_TIMER;
            event.data.timer_event.timer_id = timers[i].id;
            event.owner = timers[i].owner;

            LOG_DBG("Creating timer event: type=%d, id=%d, for owner %p", event.type, event.data.timer_event.timer_id,
                    (void *)timers[i].owner);
            LOG_DBG("Event address: %p, Queue buffer: %p", (void *)&event, (void *)ocre_event_queue_buffer_ptr);
            k_spinlock_key_t key = k_spin_lock(&ocre_event_queue_lock);
            if (k_msgq_put(&ocre_event_queue, &event, K_NO_WAIT) != 0) {
                LOG_ERR("Failed to queue timer event for timer %d", timers[i].id);
            } else {
                LOG_DBG("Queued timer event for timer %d", timers[i].id);
            }
            k_spin_unlock(&ocre_event_queue_lock, key);
        }
    }
}

#else /* POSIX */

static void posix_timer_signal_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig;
    (void)uc;
    if (si && si->si_value.sival_int > 0 && si->si_value.sival_int <= CONFIG_MAX_TIMERS) {
        posix_send_timer_event(si->si_value.sival_int);
    }
}

static void posix_send_timer_event(int timer_id) {
    if (!timer_system_initialized || !common_initialized || !ocre_event_queue_initialized) {
        LOG_ERR("Timer, common, or event queue not initialized, skipping callback");
        return;
    }
    
    int index = timer_id - 1;
    if (index < 0 || index >= CONFIG_MAX_TIMERS || !timers[index].in_use) {
        LOG_ERR("Invalid timer ID %d in callback", timer_id);
        return;
    }
    
    ocre_event_t event;
    event.type = OCRE_RESOURCE_TYPE_TIMER;
    event.data.timer_event.timer_id = timer_id;
    event.owner = timers[index].owner;
    
    LOG_DBG("Creating timer event: type=%d, id=%d, for owner %p", event.type, timer_id, (void *)timers[index].owner);
    
    posix_spinlock_key_t key = OCRE_SPINLOCK_LOCK(&ocre_event_queue_lock);
    if (posix_msgq_put(&ocre_event_queue, &event) != 0) {
        LOG_ERR("Failed to queue timer event for timer %d", timer_id);
    } else {
        LOG_DBG("Queued timer event for timer %d", timer_id);
    }
    OCRE_SPINLOCK_UNLOCK(&ocre_event_queue_lock, key);
}

#endif

/* ========== OCRE MESSAGING FUNCTIONALITY ========== */

#define CONFIG_MESSAGING_MAX_SUBSCRIPTIONS 32
#define OCRE_MAX_TOPIC_LEN 64

/* Messaging subscription structure */
typedef struct {
    char topic[OCRE_MAX_TOPIC_LEN];
    wasm_module_inst_t module_inst;
    bool is_active;
} ocre_messaging_subscription_t;

typedef struct {
    ocre_messaging_subscription_t subscriptions[CONFIG_MESSAGING_MAX_SUBSCRIPTIONS];
    uint16_t subscription_count;
#ifdef CONFIG_ZEPHYR
    struct k_mutex mutex;
#else
    pthread_mutex_t mutex;
#endif
} ocre_messaging_system_t;

static ocre_messaging_system_t messaging_system = {0};
static bool messaging_system_initialized = false;

/* Initialize messaging system */
int ocre_messaging_init(void) {
    if (messaging_system_initialized) {
        LOG_INF("Messaging system already initialized");
        return 0;
    }
    
    memset(&messaging_system, 0, sizeof(ocre_messaging_system_t));
    
#ifdef CONFIG_ZEPHYR
    k_mutex_init(&messaging_system.mutex);
#else
    pthread_mutex_init(&messaging_system.mutex, NULL);
#endif
    
    ocre_register_cleanup_handler(OCRE_RESOURCE_TYPE_MESSAGING, ocre_messaging_cleanup_container);
    messaging_system_initialized = true;
    LOG_INF("Messaging system initialized");
    return 0;
}

/* Cleanup messaging resources for a module */
void ocre_messaging_cleanup_container(wasm_module_inst_t module_inst) {
    if (!messaging_system_initialized || !module_inst) {
        return;
    }
    
#ifdef CONFIG_ZEPHYR
    k_mutex_lock(&messaging_system.mutex, K_FOREVER);
#else
    OCRE_MUTEX_LOCK(&messaging_system.mutex);
#endif
    
    for (int i = 0; i < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS; i++) {
        if (messaging_system.subscriptions[i].is_active && 
            messaging_system.subscriptions[i].module_inst == module_inst) {
            messaging_system.subscriptions[i].is_active = false;
            messaging_system.subscriptions[i].module_inst = NULL;
            messaging_system.subscriptions[i].topic[0] = '\0';
            messaging_system.subscription_count--;
            ocre_decrement_resource_count(module_inst, OCRE_RESOURCE_TYPE_MESSAGING);
            LOG_INF("Cleaned up subscription %d for module %p", i, (void *)module_inst);
        }
    }
    
#ifdef CONFIG_ZEPHYR
    k_mutex_unlock(&messaging_system.mutex);
#else
    OCRE_MUTEX_UNLOCK(&messaging_system.mutex);
#endif
    
    LOG_INF("Cleaned up messaging resources for module %p", (void *)module_inst);
}

/* Subscribe to a topic */
int ocre_messaging_subscribe(wasm_exec_env_t exec_env, void *topic) {
    if (!messaging_system_initialized) {
        if (ocre_messaging_init() != 0) {
            LOG_ERR("Failed to initialize messaging system");
            return -EINVAL;
        }
    }
    
    if (!topic || ((char *)topic)[0] == '\0') {
        LOG_ERR("Topic is NULL or empty");
        return -EINVAL;
    }
    
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        LOG_ERR("No module instance for exec_env");
        return -EINVAL;
    }
    
    ocre_module_context_t *ctx = ocre_get_module_context(module_inst);
    if (!ctx) {
        LOG_ERR("Module context not found for module instance %p", (void *)module_inst);
        return -EINVAL;
    }
    
#ifdef CONFIG_ZEPHYR
    k_mutex_lock(&messaging_system.mutex, K_FOREVER);
#else
    OCRE_MUTEX_LOCK(&messaging_system.mutex);
#endif
    
    // Check if already subscribed
    for (int i = 0; i < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS; i++) {
        if (messaging_system.subscriptions[i].is_active &&
            messaging_system.subscriptions[i].module_inst == module_inst &&
            strcmp(messaging_system.subscriptions[i].topic, (char *)topic) == 0) {
            LOG_INF("Already subscribed to topic: %s", (char *)topic);
#ifdef CONFIG_ZEPHYR
            k_mutex_unlock(&messaging_system.mutex);
#else
            OCRE_MUTEX_UNLOCK(&messaging_system.mutex);
#endif
            return 0;
        }
    }
    
    // Find a free slot
    for (int i = 0; i < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS; i++) {
        if (!messaging_system.subscriptions[i].is_active) {
            strncpy(messaging_system.subscriptions[i].topic, (char *)topic, OCRE_MAX_TOPIC_LEN - 1);
            messaging_system.subscriptions[i].topic[OCRE_MAX_TOPIC_LEN - 1] = '\0';
            messaging_system.subscriptions[i].module_inst = module_inst;
            messaging_system.subscriptions[i].is_active = true;
            messaging_system.subscription_count++;
            ocre_increment_resource_count(module_inst, OCRE_RESOURCE_TYPE_MESSAGING);
            LOG_INF("Subscribed to topic: %s, module: %p", (char *)topic, (void *)module_inst);
#ifdef CONFIG_ZEPHYR
            k_mutex_unlock(&messaging_system.mutex);
#else
            OCRE_MUTEX_UNLOCK(&messaging_system.mutex);
#endif
            return 0;
        }
    }
    
#ifdef CONFIG_ZEPHYR
    k_mutex_unlock(&messaging_system.mutex);
#else
    OCRE_MUTEX_UNLOCK(&messaging_system.mutex);
#endif
    
    LOG_ERR("No free subscription slots available");
    return -ENOMEM;
}

/* Publish a message */
int ocre_messaging_publish(wasm_exec_env_t exec_env, void *topic, void *content_type, void *payload, int payload_len) {
    if (!messaging_system_initialized) {
        if (ocre_messaging_init() != 0) {
            LOG_ERR("Failed to initialize messaging system");
            return -EINVAL;
        }
    }
    
    if (!topic || ((char *)topic)[0] == '\0') {
        LOG_ERR("Topic is NULL or empty");
        return -EINVAL;
    }
    if (!content_type || ((char *)content_type)[0] == '\0') {
        LOG_ERR("Content type is NULL or empty");
        return -EINVAL;
    }
    if (!payload || payload_len <= 0) {
        LOG_ERR("Payload is NULL or payload_len is invalid");
        return -EINVAL;
    }
    
    wasm_module_inst_t publisher_module = wasm_runtime_get_module_inst(exec_env);
    if (!publisher_module) {
        LOG_ERR("No module instance for exec_env");
        return -EINVAL;
    }
    
    static uint32_t message_id = 0;
    bool message_sent = false;
    
#ifdef CONFIG_ZEPHYR
    k_mutex_lock(&messaging_system.mutex, K_FOREVER);
#else
    OCRE_MUTEX_LOCK(&messaging_system.mutex);
#endif
    
    // Find matching subscriptions
    for (int i = 0; i < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS; i++) {
        if (!messaging_system.subscriptions[i].is_active) {
            continue;
        }
        
        // Check if the published topic matches the subscription (prefix match)
        const char *subscribed_topic = messaging_system.subscriptions[i].topic;
        size_t subscribed_len = strlen(subscribed_topic);
        
        if (strncmp(subscribed_topic, (char *)topic, subscribed_len) != 0) {
            continue; // No prefix match
        }
        
        wasm_module_inst_t target_module = messaging_system.subscriptions[i].module_inst;
        if (!target_module) {
            LOG_ERR("Invalid module instance for subscription %d", i);
            continue;
        }
        
        // Allocate WASM memory for the target module
        uint32_t topic_offset = (uint32_t)wasm_runtime_module_dup_data(target_module, (char *)topic, strlen((char *)topic) + 1);
        if (topic_offset == 0) {
            LOG_ERR("Failed to allocate WASM memory for topic");
            continue;
        }
        
        uint32_t content_offset = (uint32_t)wasm_runtime_module_dup_data(target_module, (char *)content_type, strlen((char *)content_type) + 1);
        if (content_offset == 0) {
            LOG_ERR("Failed to allocate WASM memory for content_type");
            wasm_runtime_module_free(target_module, topic_offset);
            continue;
        }
        
        uint32_t payload_offset = (uint32_t)wasm_runtime_module_dup_data(target_module, payload, payload_len);
        if (payload_offset == 0) {
            LOG_ERR("Failed to allocate WASM memory for payload");
            wasm_runtime_module_free(target_module, topic_offset);
            wasm_runtime_module_free(target_module, content_offset);
            continue;
        }
        
        // Create and queue the messaging event
        ocre_event_t event;
        event.type = OCRE_RESOURCE_TYPE_MESSAGING;
        event.data.messaging_event.message_id = message_id;
        event.data.messaging_event.topic = topic;
        event.data.messaging_event.topic_offset = topic_offset;
        event.data.messaging_event.content_type = content_type;
        event.data.messaging_event.content_type_offset = content_offset;
        event.data.messaging_event.payload = payload;
        event.data.messaging_event.payload_offset = payload_offset;
        event.data.messaging_event.payload_len = (uint32_t)payload_len;
        event.owner = target_module;
        
        LOG_DBG("Creating messaging event: ID=%d, topic=%s, content_type=%s, payload_len=%d for module %p",
                message_id, (char *)topic, (char *)content_type, payload_len, (void *)target_module);
        
#ifdef CONFIG_ZEPHYR
        k_spinlock_key_t key = k_spin_lock(&ocre_event_queue_lock);
        if (k_msgq_put(&ocre_event_queue, &event, K_NO_WAIT) != 0) {
            LOG_ERR("Failed to queue messaging event for message ID %d", message_id);
            wasm_runtime_module_free(target_module, topic_offset);
            wasm_runtime_module_free(target_module, content_offset);
            wasm_runtime_module_free(target_module, payload_offset);
        } else {
            message_sent = true;
            LOG_DBG("Queued messaging event for message ID %d", message_id);
        }
        k_spin_unlock(&ocre_event_queue_lock, key);
#else
        posix_spinlock_key_t key = OCRE_SPINLOCK_LOCK(&ocre_event_queue_lock);
        if (posix_msgq_put(&ocre_event_queue, &event) != 0) {
            LOG_ERR("Failed to queue messaging event for message ID %d", message_id);
            wasm_runtime_module_free(target_module, topic_offset);
            wasm_runtime_module_free(target_module, content_offset);
            wasm_runtime_module_free(target_module, payload_offset);
        } else {
            message_sent = true;
            LOG_DBG("Queued messaging event for message ID %d", message_id);
        }
        OCRE_SPINLOCK_UNLOCK(&ocre_event_queue_lock, key);
#endif
    }
    
#ifdef CONFIG_ZEPHYR
    k_mutex_unlock(&messaging_system.mutex);
#else
    OCRE_MUTEX_UNLOCK(&messaging_system.mutex);
#endif
    
    if (message_sent) {
        LOG_DBG("Published message: ID=%d, topic=%s, content_type=%s, payload_len=%d", 
                message_id, (char *)topic, (char *)content_type, payload_len);
        message_id++;
        return 0;
    } else {
        LOG_ERR("No matching subscriptions found for topic %s", (char *)topic);
        return -ENOENT;
    }
}

/* Free module event data */
int ocre_messaging_free_module_event_data(wasm_exec_env_t exec_env, uint32_t topic_offset, uint32_t content_offset, uint32_t payload_offset) {
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        LOG_ERR("Cannot find module_inst for free event data");
        return -EINVAL;
    }

    wasm_runtime_module_free(module_inst, topic_offset);
    wasm_runtime_module_free(module_inst, content_offset);
    wasm_runtime_module_free(module_inst, payload_offset);

    return 0;
}
