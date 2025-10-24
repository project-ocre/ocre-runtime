/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-License: Apache-2.0
 */

#ifndef OCRE_COMMON_H
#define OCRE_COMMON_H

#include <wasm_export.h>
#include <stdbool.h>
#include <stdint.h>

/* Platform-specific includes */
#ifdef CONFIG_ZEPHYR
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include "../ocre_messaging/ocre_messaging.h"
#else
#include <pthread.h>
/* Forward declarations for POSIX messaging types */
struct ocre_messaging_event;
#endif

#define OCRE_EVENT_THREAD_STACK_SIZE 2048
#define OCRE_EVENT_THREAD_PRIORITY   5
#define OCRE_WASM_STACK_SIZE         16384
#define EVENT_THREAD_POOL_SIZE 0

extern bool common_initialized;
extern bool ocre_event_queue_initialized;
extern __thread wasm_module_inst_t *current_module_tls;
extern char *ocre_event_queue_buffer_ptr;       // Defined in ocre_common.c

/* Platform-specific external declarations */
#ifdef CONFIG_ZEPHYR
extern struct k_msgq ocre_event_queue;          // Defined in ocre_common.c
extern struct k_spinlock ocre_event_queue_lock; // Defined in ocre_common.c
#else
/* POSIX equivalents will be defined in the .c file */
#endif


/**
 * @brief Enumeration of OCRE resource types.
 */
typedef enum {
    OCRE_RESOURCE_TYPE_TIMER,     ///< Timer resource
    OCRE_RESOURCE_TYPE_GPIO,      ///< GPIO resource
    OCRE_RESOURCE_TYPE_SENSOR,    ///< Sensor resource 
    OCRE_RESOURCE_TYPE_MESSAGING, ///< Messaging resource
    OCRE_RESOURCE_TYPE_COUNT      ///< Total number of resource types
} ocre_resource_type_t;

/**
 * @brief Structure representing the context of an OCRE module.
 */
typedef struct {
    wasm_module_inst_t inst;                                    ///< WASM module instance
    wasm_exec_env_t exec_env;                                   ///< WASM execution environment
    bool in_use;                                                ///< Flag indicating if the module is in use
    uint32_t last_activity;                                     ///< Timestamp of the last activity
    uint32_t resource_count[OCRE_RESOURCE_TYPE_COUNT];          ///< Count of resources per type
    wasm_function_inst_t dispatchers[OCRE_RESOURCE_TYPE_COUNT]; ///< Event dispatchers per resource type
} ocre_module_context_t;

/**
 * @brief Type definition for cleanup handler function.
 *
 * @param module_inst The WASM module instance to clean up.
 */
typedef void (*ocre_cleanup_handler_t)(wasm_module_inst_t module_inst);

/**
 * @brief Structure representing an OCRE event for dispatching.
 */
typedef struct {
    union {
        struct {
            uint32_t timer_id;        ///< Timer ID
        } timer_event;                ///< Timer event data
        struct {
            uint32_t pin_id;          ///< GPIO pin ID
            uint32_t port;           ///< GPIO port 
            uint32_t state;           ///< GPIO state
        } gpio_event;                 ///< GPIO event data
        struct {
            uint32_t sensor_id;       ///< Sensor ID
            uint32_t channel;         ///< Sensor channel
            uint32_t value;           ///< Sensor value
        } sensor_event;               ///< Sensor event data
        struct {
            uint32_t message_id;                ///< Message ID
            char *topic;                        ///< Message topic
            uint32_t topic_offset;              ///< Message topic offset
            char *content_type;                 ///< Message content type
            uint32_t content_type_offset;       ///< Message content type offset
            void *payload;                      ///< Message payload
            uint32_t payload_offset;            ///< Message payload offset
            uint32_t payload_len;               ///< Payload length
        } messaging_event;            ///< Messaging event data
        /*
            =============================
            Place to add more event data  
            =============================
        */
    } data;                           ///< Union of event data
    wasm_module_inst_t owner;           ///< Owner module instance
    ocre_resource_type_t type;        ///< Type of the event
} ocre_event_t;

/**
 * @brief Initialize the OCRE common system.
 *
 * This function initializes the common components required for OCRE operations.
 *
 * @return 0 on success, negative error code on failure.
 */
int ocre_common_init(void);

/**
 * @brief Register a WASM module with the OCRE system.
 *
 * @param module_inst The WASM module instance to register.
 * @return 0 on success, negative error code on failure.
 */
int ocre_register_module(wasm_module_inst_t module_inst);

/**
 * @brief Unregister a WASM module from the OCRE system.
 *
 * @param module_inst The WASM module instance to unregister.
 */
void ocre_unregister_module(wasm_module_inst_t module_inst);

/**
 * @brief Get the context of a registered WASM module.
 *
 * @param module_inst The WASM module instance.
 * @return Pointer to the module context, or NULL if not found.
 */
ocre_module_context_t *ocre_get_module_context(wasm_module_inst_t module_inst);

/**
 * @brief Register an event dispatcher for a specific resource type.
 *
 * @param exec_env WASM execution environment.
 * @param type Resource type.
 * @param function_name Name of the WASM function to use as dispatcher.
 * @return 0 on success, negative error code on failure.
 */
int ocre_register_dispatcher(wasm_exec_env_t exec_env, ocre_resource_type_t type, const char *function_name);

/**
 * @brief Get the count of resources of a specific type for a module.
 *
 * @param module_inst The WASM module instance.
 * @param type Resource type.
 * @return Number of resources, or 0 if module not found.
 */
uint32_t ocre_get_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type);

/**
 * @brief Increment the resource count for a specific type.
 *
 * @param module_inst The WASM module instance.
 * @param type Resource type.
 */
void ocre_increment_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type);

/**
 * @brief Decrement the resource count for a specific type.
 *
 * @param module_inst The WASM module instance.
 * @param type Resource type.
 */
void ocre_decrement_resource_count(wasm_module_inst_t module_inst, ocre_resource_type_t type);

/**
 * @brief Clean up resources for a module.
 *
 * @param module_inst The WASM module instance.
 */
void ocre_cleanup_module_resources(wasm_module_inst_t module_inst);

/**
 * @brief Register a cleanup handler for a resource type.
 *
 * @param type Resource type.
 * @param handler Cleanup handler function.
 * @return 0 on success, negative error code on failure.
 */
int ocre_register_cleanup_handler(ocre_resource_type_t type, ocre_cleanup_handler_t handler);

/**
 * @brief Get the current WASM module instance.
 *
 * @return The current WASM module instance, or NULL if not set.
 */
wasm_module_inst_t ocre_get_current_module(void);

/**
 * @brief Get an event from the WASM event queue.
 *
 * @param exec_env WASM execution environment.
 * @param type_offset Offset in WASM memory for event type.
 * @param id_offset Offset in WASM memory for event ID.
 * @param port_offset Offset in WASM memory for event port.
 * @param state_offset Offset in WASM memory for event state.
 * @param extra_offset Offset in WASM memory for extra data.
 * @param payload_len_offset Offset in WASM memory for payload length.
 * @return 0 on success, negative error code on failure.
 */
int ocre_get_event(wasm_exec_env_t exec_env, uint32_t type_offset, uint32_t id_offset, uint32_t port_offset,
                   uint32_t state_offset, uint32_t extra_offset, uint32_t payload_len_offset);

void ocre_common_shutdown(void);

/* ========== OCRE TIMER FUNCTIONALITY ========== */

#ifndef OCRE_TIMER_T_DEFINED
#define OCRE_TIMER_T_DEFINED
typedef uint32_t ocre_timer_t;
#endif

/**
 * @brief Initialize the timer system.
 */
void ocre_timer_init(void);

/**
 * @brief Create a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @return 0 on success, negative error code on failure.
 */
int ocre_timer_create(wasm_exec_env_t exec_env, int id);

/**
 * @brief Delete a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @return 0 on success, negative error code on failure.
 */
int ocre_timer_delete(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Start a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @param interval Timer interval in milliseconds.
 * @param is_periodic Whether the timer is periodic.
 * @return 0 on success, negative error code on failure.
 */
int ocre_timer_start(wasm_exec_env_t exec_env, ocre_timer_t id, int interval, int is_periodic);

/**
 * @brief Stop a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @return 0 on success, negative error code on failure.
 */
int ocre_timer_stop(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Get remaining time for a timer.
 *
 * @param exec_env WASM execution environment.
 * @param id Timer ID.
 * @return Remaining time in milliseconds, or negative error code on failure.
 */
int ocre_timer_get_remaining(wasm_exec_env_t exec_env, ocre_timer_t id);

/**
 * @brief Cleanup timer resources for a module.
 *
 * @param module_inst WASM module instance.
 */
void ocre_timer_cleanup_container(wasm_module_inst_t module_inst);

#endif /* OCRE_COMMON_H */
