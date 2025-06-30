/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <ocre/ocre.h>
#include <stdlib.h>
#include "messaging.h"
#include "ocre/utils/utils.h"

#define MAX_MSG_SIZE 4096

#include <pthread.h>
#include <mqueue.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#define QUEUE_SIZE      10
#define STACK_SIZE      1024
#define PRIORITY        5
#define WASM_STACK_SIZE (8 * 1024)

static mqd_t ocre_msg_queue;
char mq_name[32];
static pthread_t subscriber_thread_data;
static struct mq_attr mq_attr = {
    .mq_flags = 0,
    .mq_maxmsg = QUEUE_SIZE,
    .mq_msgsize = MAX_MSG_SIZE,
    .mq_curmsgs = 0
};

#define CONFIG_MESSAGING_MAX_SUBSCRIPTIONS 10

void cleanup_messaging() {
    mq_close(ocre_msg_queue);
    mq_unlink(mq_name);
    exit(0); // Terminate the process after cleanup
}


// Structure to hold the subscription information
typedef struct {
    char *topic;
    wasm_function_inst_t handler;
    wasm_module_inst_t module_inst;
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

void* subscriber_thread(void *arg) {
    wasm_runtime_init_thread_env();
    uint8_t buffer[MAX_MSG_SIZE];
    while (1) {
        ssize_t msg_size = mq_receive(ocre_msg_queue, (char *)buffer, sizeof(buffer), NULL);
        if (msg_size >= (ssize_t)sizeof(ocre_msg_t)) {
            LOG_INF("Got a message from another container");
            ocre_msg_t *msg = (ocre_msg_t *)buffer;
            char *topic = (char *)(buffer + msg->topic);
            char *content_type = (char *)(buffer + msg->content_type);
            void *payload = (void *)(buffer + msg->payload);

            for (int i = 0; i < subscription_count; i++) {
                if (strcmp(subscriptions[i].topic, topic) == 0) {
                    wasm_module_inst_t module_inst = subscriptions[i].module_inst;
                    if (!module_inst) {
                        LOG_ERR("Invalid module instance");
                        continue;
                    }

                    // Create exec_env for this thread/call
                    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, WASM_STACK_SIZE);
                    if (!exec_env) {
                        LOG_ERR("Failed to create exec_env");
                        continue;
                    }

                    uint32_t topic_offset =
                            (uint32_t)wasm_runtime_module_dup_data(module_inst, topic, strlen(topic) + 1);
                    if (topic_offset == 0) {
                        LOG_ERR("Failed to allocate memory for topic in WASM");
                        continue;
                    }
                    uint32_t content_offset = (uint32_t)wasm_runtime_module_dup_data(module_inst, content_type, strlen(content_type) + 1);
                    if (content_offset == 0) {
                        LOG_ERR("Failed to allocate memory for content_type in WASM");
                        wasm_runtime_module_free(module_inst, topic_offset);
                        continue;
                    }
                    uint32_t payload_offset =
                            (uint32_t)wasm_runtime_module_dup_data(module_inst, payload, msg->payload_len);
                    if (payload_offset == 0) {
                        LOG_ERR("Failed to allocate memory for payload in WASM");
                        wasm_runtime_module_free(module_inst, topic_offset);
                        wasm_runtime_module_free(module_inst, content_offset);
                        continue;
                    }

                    uint32_t args[5] = {msg->mid, topic_offset, content_offset, payload_offset, msg->payload_len};
                    if (!wasm_runtime_call_wasm(exec_env, subscriptions[i].handler, 5, args)) {
                        const char *error = wasm_runtime_get_exception(module_inst);
                        LOG_ERR("Failed to call WASM function: %s", error ? error : "Unknown error");
                    } else {
                        LOG_INF("Function executed successfully");
                    }

                    // Free memory and exec_env
                    wasm_runtime_module_free(module_inst, topic_offset);
                    wasm_runtime_module_free(module_inst, content_offset);
                    wasm_runtime_module_free(module_inst, payload_offset);
                    wasm_runtime_destroy_exec_env(exec_env);
                }
            }
        } else {
            core_sleep_ms(1000);
        }
    }
    printf("do I get here?\n");
    wasm_runtime_destroy_thread_env();
    return NULL;
}

void ocre_msg_system_init() {
    if (msg_system_is_init) {
        LOG_WRN("Messaging System is already initialized");
        return;
    }

    // POSIX: Create message queue and thread
    snprintf(mq_name, sizeof(mq_name), "/ocre_msgq_%d", getpid());
    ocre_msg_queue = mq_open(mq_name, O_CREAT | O_RDWR, 0644, &mq_attr);
    if (ocre_msg_queue == (mqd_t)-1) {
        perror("Failed to create message queue");
        return;
    }
    if (pthread_create(&subscriber_thread_data, NULL, subscriber_thread, NULL) != 0) {
        perror("Failed to create subscriber thread");
        mq_close(ocre_msg_queue);
        mq_unlink(mq_name); // Clean up if thread creation fails
        return;
    }
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

    size_t topic_len = strlen(topic) + 1;
    size_t content_type_len = strlen(content_type) + 1;
    size_t total_size = sizeof(ocre_msg_t) + topic_len + content_type_len + payload_len;

    if (total_size > MAX_MSG_SIZE) {
        LOG_ERR("Message too large for queue: %zu > %d", total_size, MAX_MSG_SIZE);
        return -1;
    }

    uint8_t *buffer = malloc(total_size);
    if (!buffer) {
        LOG_ERR("Failed to allocate message buffer");
        return -1;
    }

    ocre_msg_t *msg = (ocre_msg_t *)buffer;
    msg->mid = message_id++;
    msg->topic = sizeof(ocre_msg_t);
    msg->content_type = msg->topic + topic_len;
    msg->payload = msg->content_type + content_type_len;
    msg->payload_len = payload_len;

    memcpy(buffer + msg->topic, topic, topic_len);
    memcpy(buffer + msg->content_type, content_type, content_type_len);
    memcpy(buffer + msg->payload, payload, payload_len);

    if (mq_send(ocre_msg_queue, (const char *)buffer, total_size, 0) != 0) {
        perror("Message queue is full or error occurred");
        free(buffer);
        return -1;
    }
    free(buffer);

    return 0;
}

int ocre_subscribe_message(wasm_exec_env_t exec_env, char *topic, char *handler_name) {
    LOG_INF("---------------_ocre_subscribe_message---------------");

    if (!msg_system_is_init) {
        LOG_ERR("Messaging system not initialized");
        return -1;
    }

    signal(SIGINT, cleanup_messaging);
    signal(SIGTERM, cleanup_messaging);

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
        subscriptions[subscription_count].module_inst = current_module_inst;

        LOG_INF("WASM messaging callback function set successfully");

        subscription_count++;
    } else {
        LOG_ERR("Maximum subscriptions reached, could not subscribe to topic");
        return -1;
    }

    return 0;
}
