
#include <zephyr/kernel.h>
#include <string.h>
#include <ocre/ocre.h>
#include <stdlib.h>
#include "messaging.h"

LOG_MODULE_DECLARE(ocre_container_messaging, OCRE_LOG_LEVEL);

#define QUEUE_SIZE      100
#define STACK_SIZE      1024
#define PRIORITY        5
#define WASM_STACK_SIZE (8 * 1024)

K_MSGQ_DEFINE(ocre_msg_queue, sizeof(ocre_msg_t), QUEUE_SIZE, 4);

// Structure to hold the subscription information
typedef struct {
    char *topic;
    wasm_function_inst_t handler;
    wasm_module_inst_t module_inst;
    wasm_exec_env_t exec_env;
} ocre_subscription_t;

static ocre_subscription_t subscriptions[CONFIG_MESSAGING_MAX_SUBSCRIPTIONS];
static int subscription_count = 0;
static bool msg_system_is_init = false;

static uint32_t allocate_wasm_memory(wasm_module_inst_t module_inst, const void *src, size_t size) {
    void *native_addr = NULL;
    uint64_t wasm_ptr = wasm_runtime_module_malloc(module_inst, size, &native_addr);
    if (!wasm_ptr || !native_addr) {
        LOG_ERR("Failed to allocate memory in WASM");
        return 0;
    }
    if (src) {
        memcpy(native_addr, src, size);
    }
    return (uint32_t)wasm_ptr;
}

static void free_wasm_message(wasm_module_inst_t module_inst, uint32_t *ptr_array, uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        if (ptr_array[i]) {
            wasm_runtime_module_free(module_inst, ptr_array[i]);
        }
    }
}

void subscriber_thread(void *arg1, void *arg2, void *arg3) {
    ocre_msg_t msg;
    while (1) {
        if (k_msgq_get(&ocre_msg_queue, &msg, K_FOREVER) == 0) {
            for (int i = 0; i < subscription_count; i++) {
                if (strcmp(subscriptions[i].topic, msg.topic) == 0) {

                    wasm_module_inst_t module_inst = subscriptions[i].module_inst;
                    if (!module_inst) {
                        LOG_ERR("Invalid module instance");
                        continue;
                    }

                    uint32_t allocated_pointers[4] = {0};
                    // Allocate memory for ocre_msg_t
                    allocated_pointers[0] = allocate_wasm_memory(module_inst, NULL, sizeof(ocre_msg_t));
                    if (allocated_pointers[0] == 0) {
                        LOG_ERR("Failed to allocate memory for message structure in WASM");
                        continue;
                    }

                    // Allocate memory for topic
                    allocated_pointers[1] = allocate_wasm_memory(module_inst, msg.topic, strlen(msg.topic) + 1);
                    if (allocated_pointers[1] == 0) {
                        LOG_ERR("Failed to allocate memory for topic in WASM");
                        free_wasm_message(module_inst, allocated_pointers, 1);
                        continue;
                    }

                    // Allocate memory for content_type
                    allocated_pointers[2] =
                            allocate_wasm_memory(module_inst, msg.content_type, strlen(msg.content_type) + 1);
                    if (allocated_pointers[2] == 0) {
                        LOG_ERR("Failed to allocate memory for content_type in WASM");
                        free_wasm_message(module_inst, allocated_pointers, 2);
                        continue;
                    }

                    // Allocate memory for payload
                    allocated_pointers[3] = allocate_wasm_memory(module_inst, msg.payload, msg.payload_len);
                    if (allocated_pointers[3] == 0) {
                        LOG_ERR("Failed to allocate memory for payload in WASM");
                        free_wasm_message(module_inst, allocated_pointers, 3);
                        continue;
                    }

                    // Populate the WASM message structure
                    ocre_msg_t *wasm_msg =
                            (ocre_msg_t *)wasm_runtime_addr_app_to_native(module_inst, allocated_pointers[0]);
                    wasm_msg->mid = msg.mid;
                    wasm_msg->topic = (char *)allocated_pointers[1];
                    wasm_msg->content_type = (char *)allocated_pointers[2];
                    wasm_msg->payload = (void *)allocated_pointers[3];
                    wasm_msg->payload_len = msg.payload_len;

                    // Call the WASM function
                    uint32_t args[1] = {allocated_pointers[0]};
                    if (!wasm_runtime_call_wasm(subscriptions[i].exec_env, subscriptions[i].handler, 1, args)) {
                        const char *error = wasm_runtime_get_exception(module_inst);
                        LOG_ERR("Failed to call WASM function: %s", error ? error : "Unknown error");
                    } else {
                        LOG_INF("Function executed successfully");
                    }

                    // Free allocated WASM memory
                    free_wasm_message(module_inst, allocated_pointers, 4);
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

    if (!tid) {
        LOG_ERR("Failed to create container messaging thread");
        return;
    }
    k_thread_name_set(tid, "container_messaging");
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

    static uint64_t message_id = 0;
    ocre_msg_t msg = {.mid = message_id,
                      .topic = topic,
                      .content_type = content_type,
                      .payload = payload,
                      .payload_len = payload_len};

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

    if (subscription_count < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS) {

        if (!topic || (topic[0] == '\0')) {
            LOG_ERR("topic is NULL or empty, please check function parameters!");
            return -1;
        }

        if (!handler_name || (handler_name[0] == '\0')) {
            LOG_ERR("handler_name is NULL or empty, please check function parameters!");
            return -1;
        }

        wasm_module_inst_t current_module_inst = wasm_runtime_get_module_inst(exec_env);
        wasm_function_inst_t handler_function = wasm_runtime_lookup_function(current_module_inst, handler_name);
        if (!handler_function) {
            LOG_ERR("Failed to find %s in WASM module", handler_name);
            return -1;
        }

        subscriptions[subscription_count].topic = topic;
        subscriptions[subscription_count].handler = handler_function;
        subscriptions[subscription_count].exec_env = exec_env;
        subscriptions[subscription_count].module_inst = current_module_inst;

        LOG_INF("WASM messaging callback function set successfully");

        subscription_count++;
    } else {
        LOG_ERR("Maximum subscriptions reached, could not subscribe to topic");
        return -1;
    }

    return 0;
}
