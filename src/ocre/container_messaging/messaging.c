
#include <zephyr/kernel.h>
#include <string.h>
#include <ocre/ocre.h>
#include <stdlib.h>
#include "messaging.h"

LOG_MODULE_DECLARE(ocre_container_messaging, OCRE_LOG_LEVEL);

#define MESSAGING_MAX_MODULE_ENVS CONFIG_MAX_CONTAINERS

#define QUEUE_SIZE      100
#define STACK_SIZE      4096
#define PRIORITY        5
#define WASM_STACK_SIZE (8 * 1024)

K_MSGQ_DEFINE(ocre_msg_queue, sizeof(ocre_msg_t), QUEUE_SIZE, 4);

// Structure to hold the subscription information
typedef struct {
    char *topic;
    wasm_function_inst_t handler;
    wasm_module_inst_t module_inst;
    wasm_exec_env_t exec_env;
    bool is_active;
} messaging_subscription_t;

typedef struct {
    messaging_subscription_t info[CONFIG_MESSAGING_MAX_SUBSCRIPTIONS];
    uint16_t subscriptions_number;
} messaging_subscription_list;

// Structure to hold all module environments
typedef struct {
    wasm_module_inst_t module_inst;
    wasm_exec_env_t exec_env;
    bool is_active;
} messaging_module_env_t;

typedef struct {
    messaging_module_env_t module_info[MESSAGING_MAX_MODULE_ENVS];
    uint16_t envs_number;
} messaging_module_env_list;

messaging_subscription_list subscription_list = {0};
messaging_module_env_list module_env_list = {0};

static bool msg_system_is_init = false;

void subscriber_thread(void *arg1, void *arg2, void *arg3) {
    ocre_msg_t msg;

    while (1) {
        if (k_msgq_get(&ocre_msg_queue, &msg, K_FOREVER) == 0) {
            for (int i = 0; i < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS; i++) {
                if (!subscription_list.info[i].is_active) {
                    continue;
                }
                if (strcmp(subscription_list.info[i].topic, msg.topic) == 0) {

                    wasm_module_inst_t module_inst = subscription_list.info[i].module_inst;
                    if (!module_inst) {
                        LOG_ERR("Invalid module instance");
                        continue;
                    }

                    // Allocate memory for topic
                    uint32_t topic_offset =
                            (uint32_t)wasm_runtime_module_dup_data(module_inst, msg.topic, strlen(msg.topic) + 1);
                    if (topic_offset == 0) {
                        LOG_ERR("Failed to allocate memory for topic in WASM");
                        continue;
                    }

                    // Allocate memory for content_type
                    uint32_t content_offset = (uint32_t)wasm_runtime_module_dup_data(module_inst, msg.content_type,
                                                                                     strlen(msg.content_type) + 1);
                    if (content_offset == 0) {
                        LOG_ERR("Failed to allocate memory for content_type in WASM");
                        wasm_runtime_module_free(module_inst, topic_offset);
                        continue;
                    }

                    // Allocate memory for payload
                    uint32_t payload_offset =
                            (uint32_t)wasm_runtime_module_dup_data(module_inst, msg.payload, msg.payload_len);
                    if (payload_offset == 0) {
                        LOG_ERR("Failed to allocate memory for payload in WASM");
                        wasm_runtime_module_free(module_inst, topic_offset);
                        wasm_runtime_module_free(module_inst, content_offset);
                        continue;
                    }

                    // Call the WASM function
                    uint32_t args[5] = {msg.mid, topic_offset, content_offset, payload_offset, msg.payload_len};
                    if (!wasm_runtime_call_wasm(subscription_list.info[i].exec_env, subscription_list.info[i].handler,
                                                5, args)) {
                        const char *error = wasm_runtime_get_exception(module_inst);
                        LOG_ERR("Failed to call WASM function: %s", error ? error : "Unknown error");
                    } else {
                        LOG_INF("Function executed successfully");
                    }

                    // Free allocated WASM memory
                    wasm_runtime_module_free(module_inst, topic_offset);
                    wasm_runtime_module_free(module_inst, content_offset);
                    wasm_runtime_module_free(module_inst, payload_offset);
                }
            }
        }
    }
}

// Define the stack area for the subscriber thread
K_THREAD_STACK_DEFINE(subscriber_stack_area, STACK_SIZE);
struct k_thread subscriber_thread_data;

// Initialize the message system
void ocre_msg_system_init() {
    if (msg_system_is_init) {
        LOG_WRN("Messaging System is already initialized");
        return;
    }

    k_tid_t tid = k_thread_create(&subscriber_thread_data, subscriber_stack_area,
                                  K_THREAD_STACK_SIZEOF(subscriber_stack_area), subscriber_thread, NULL, NULL, NULL,
                                  PRIORITY, 0, K_NO_WAIT);
    wasm_runtime_init_thread_env();
    if (!tid) {
        LOG_ERR("Failed to create container messaging thread");
        return;
    }
    k_thread_name_set(tid, "container_messaging");

    memset(&module_env_list, 0, sizeof(messaging_module_env_list));
    memset(&subscription_list, 0, sizeof(messaging_subscription_list));

    msg_system_is_init = true;
}

int ocre_publish_message(wasm_exec_env_t exec_env, char *topic, char *content_type, void *payload, int payload_len) {
    LOG_INF("---------------_ocre_publish_message: topic: %s [%zu], content_type: %s, payload_len: %u ", topic,
            strlen(topic), content_type, payload_len);

    if (!msg_system_is_init) {
        LOG_ERR("Messaging system not initialized");
        return -1;
    }

    if (!topic || (topic[0] == '\0')) {
        LOG_ERR("topic is NULL or empty, please check function parameters!");
        return -1;
    }

    if (!content_type || (content_type[0] == '\0')) {
        LOG_ERR("content_type is NULL or empty, please check function parameters!");
        return -1;
    }

    if (!payload || payload_len == 0) {
        LOG_ERR("payload is NULL or payload_len is 0, please check function parameters!");
        return -1;
    }

    static uint32_t message_id = 0;
    ocre_msg_t msg = {.mid = message_id,
                      .topic = topic,
                      .content_type = content_type,
                      .payload = payload,
                      .payload_len = (uint32_t)payload_len};

    message_id++;

    if (k_msgq_put(&ocre_msg_queue, &msg, K_NO_WAIT) != 0) {
        // Handle message queue full scenario
        LOG_ERR("Message queue is full, could not publish message\n");
        return -1;
    }

    return 0;
}

int ocre_subscribe_message(wasm_exec_env_t exec_env, char *topic, char *handler_name) {
    LOG_INF("---------------_ocre_subscribe_message---------------");

    if (!msg_system_is_init) {
        LOG_ERR("Messaging system not initialized");
        return -1;
    }

    if (subscription_list.subscriptions_number >= CONFIG_MESSAGING_MAX_SUBSCRIPTIONS) {
        LOG_ERR("Maximum subscriptions reached");
        return -1;
    }

    if (!topic || (topic[0] == '\0')) {
        LOG_ERR("topic is NULL or empty, please check function parameters!");
        return -1;
    }

    if (!handler_name || (handler_name[0] == '\0')) {
        LOG_ERR("handler_name is NULL or empty, please check function parameters!");
        return -1;
    }

    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        LOG_ERR("Failed to retrieve module instance from execution environment");
        return -1;
    }

    wasm_exec_env_t assigned_exec_env = NULL;
    for (int i = 0; i < MESSAGING_MAX_MODULE_ENVS; i++) {
        if (module_env_list.module_info[i].module_inst == module_inst) {
            assigned_exec_env = module_env_list.module_info[i].exec_env;
            break;
        }
    }

    if (!assigned_exec_env) {
        LOG_ERR("Execution environment not found for module instance");
        return -1;
    }

    wasm_function_inst_t handler_function = wasm_runtime_lookup_function(module_inst, handler_name);
    if (!handler_function) {
        LOG_ERR("Failed to find %s in WASM module", handler_name);
        return -1;
    }

    // Find a free slot for subscription and set a new subscription
    for (int i = 0; i < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS; i++) {
        if (!subscription_list.info[i].is_active) {
            subscription_list.info[i].topic = topic;
            subscription_list.info[i].handler = handler_function;
            subscription_list.info[i].exec_env = assigned_exec_env;
            subscription_list.info[i].module_inst = module_inst;
            subscription_list.info[i].is_active = true;

            subscription_list.subscriptions_number++;
            break;
        }
    }

    LOG_INF("WASM messaging callback function set successfully");
    return 0;
}

void ocre_messaging_register_module(wasm_module_inst_t module_inst) {
    if (module_env_list.envs_number >= MESSAGING_MAX_MODULE_ENVS) {
        LOG_ERR("Maximum module instances reached");
        return;
    }

    if (!module_inst) {
        LOG_ERR("Module instance is NULL");
        return;
    }

    // Check if module already exists
    for (int i = 0; i < MESSAGING_MAX_MODULE_ENVS; i++) {
        if (module_env_list.module_info[i].module_inst == module_inst) {
            LOG_WRN("Module instance already set");
            return;
        }
    }

    // Create new exec env
    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, WASM_STACK_SIZE);
    if (!exec_env) {
        LOG_ERR("Failed to create execution environment");
        return;
    }

    // Save module_inst and exec_env
    for (int i = 0; i < MESSAGING_MAX_MODULE_ENVS; i++) {
        if (!module_env_list.module_info[i].is_active) {
            module_env_list.module_info[i].module_inst = module_inst;
            module_env_list.module_info[i].exec_env = exec_env;
            module_env_list.module_info[i].is_active = true;
            module_env_list.envs_number++;
            break;
        }
    }

    LOG_INF("Module instance registered successfully");
}

void ocre_messaging_cleanup_container(wasm_module_inst_t module_inst) {
    if (!module_inst) {
        LOG_ERR("Module instance is NULL");
        return;
    }

    /* Stop and clear subscribers related to this module_inst */
    for (int i = 0; i < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS; i++) {
        if (subscription_list.info[i].module_inst == module_inst && subscription_list.info[i].is_active) {
            LOG_DBG("Cleaning up subscription: %s", subscription_list.info[i].topic);
            memset(&subscription_list.info[i], 0, sizeof(messaging_subscription_t));
            subscription_list.subscriptions_number--;
        }
    }
    /* Destroy and clear module environments related to this module_inst */
    for (int i = 0; i < MESSAGING_MAX_MODULE_ENVS; i++) {
        if (module_env_list.module_info[i].module_inst == module_inst && module_env_list.module_info[i].is_active) {
            if (module_env_list.module_info[i].exec_env) {
                wasm_runtime_destroy_exec_env(module_env_list.module_info[i].exec_env);
            }
            memset(&module_env_list.module_info[i], 0, sizeof(messaging_module_env_t));
            module_env_list.envs_number--;
        }
    }
    wasm_runtime_destroy_thread_env();

    LOG_INF("Cleaned up messaging for module %p", (void *)module_inst);
}
