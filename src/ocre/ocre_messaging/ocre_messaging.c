/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre, a Series of LF Projects, LLC
 *
 * SPDX-License-License: Apache-2.0
 */

#include <ocre/ocre.h>
#include <ocre/api/ocre_common.h>
#include <ocre/ocre_messaging/ocre_messaging.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <string.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#define MESSAGING_MAX_SUBSCRIPTIONS CONFIG_MESSAGING_MAX_SUBSCRIPTIONS

// Structure to hold the subscription information
typedef struct {
    void *topic; // Topic pointer
    wasm_module_inst_t module_inst;
    bool is_active;
} messaging_subscription_t;

typedef struct {
    messaging_subscription_t info[MESSAGING_MAX_SUBSCRIPTIONS];
    uint16_t subscriptions_number;
} messaging_subscription_list;

static messaging_subscription_list subscription_list = {0};
static bool messaging_system_initialized = false;

int ocre_messaging_init(void) {
    if (messaging_system_initialized) {
        LOG_INF("Messaging system already initialized");
        return -1;
    }
    if (!common_initialized && ocre_common_init() != 0) {
        LOG_ERR("Failed to initialize common subsystem");
        return -2;
    }

    memset(&subscription_list, 0, sizeof(messaging_subscription_list));
    ocre_register_cleanup_handler(OCRE_RESOURCE_TYPE_MESSAGING, ocre_messaging_cleanup_container);
    messaging_system_initialized = true;

    LOG_INF("Messaging system initialized");
    return 0;
}

int ocre_messaging_publish(wasm_exec_env_t exec_env, void *topic, void *content_type, void *payload, int payload_len) {
    if (!messaging_system_initialized) {
        LOG_ERR("Messaging system not initialized");
        return -EINVAL;
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

    if ((uintptr_t)ocre_event_queue_buffer_ptr % 4 != 0) {
        LOG_ERR("ocre_event_queue_buffer misaligned: %p", (void *)ocre_event_queue_buffer_ptr);
        return -EPIPE;
    }
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    if (!module_inst) {
        LOG_ERR("No module instance for exec_env");
        return -EINVAL;
    }
    static uint32_t message_id = 0;
    bool posted = false;

    // Iterate through subscriptions to find matching topics
    for (int i = 0; i < MESSAGING_MAX_SUBSCRIPTIONS; i++) {
        if (!subscription_list.info[i].is_active ||
            strcmp((char *)subscription_list.info[i].topic, (char *)topic) != 0) {
            continue;
        }
        wasm_module_inst_t target_module = subscription_list.info[i].module_inst;
        if (!target_module) {
            LOG_ERR("Invalid module instance for subscription %d", i);
            continue;
        }

        // Allocate WASM memory for topic, content_type, and payload
        uint32_t topic_offset =
                (uint32_t)wasm_runtime_module_dup_data(target_module, (char *)topic, strlen((char *)topic) + 1);
        if (topic_offset == 0) {
            LOG_ERR("Failed to allocate WASM memory for topic");
            continue;
        }
        uint32_t content_offset = (uint32_t)wasm_runtime_module_dup_data(target_module, (char *)content_type,
                                                                         strlen((char *)content_type) + 1);
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
        event.data.messaging_event.owner = target_module;

        k_spinlock_key_t key = k_spin_lock(&ocre_event_queue_lock);
        if (k_msgq_put(&ocre_event_queue, &event, K_NO_WAIT) != 0) {
            LOG_ERR("Failed to queue messaging event for message ID %d", message_id);
            wasm_runtime_module_free(target_module, topic_offset);
            wasm_runtime_module_free(target_module, content_offset);
            wasm_runtime_module_free(target_module, payload_offset);
        } else {
            posted = true;
            LOG_INF("Queued messaging event: ID=%d, topic=%s, content_type=%s, payload_len=%d for module %p",
                    message_id, (char *)topic, (char *)content_type, payload_len, (void *)target_module);
        }
        k_spin_unlock(&ocre_event_queue_lock, key);
    }
    if (posted) {
        LOG_INF("Published message: ID=%d, topic=%s, content_type=%s, payload_len=%d", message_id, (char *)topic,
                (char *)content_type, payload_len);
        message_id++;
        return 0;
    } else {
        LOG_ERR("No matching subscriptions found for topic %s", (char *)topic);
        return -ENOENT;
    }
}

int ocre_messaging_subscribe(wasm_exec_env_t exec_env, void *topic) {
    if (!messaging_system_initialized) {
        LOG_ERR("Messaging system not initialized");
        return -EINVAL;
    }
    if (subscription_list.subscriptions_number >= MESSAGING_MAX_SUBSCRIPTIONS) {
        LOG_ERR("Maximum subscriptions reached");
        return -ENOMEM;
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
    if (!ctx || !ctx->exec_env) {
        LOG_ERR("Execution environment not found for module instance %p", (void *)module_inst);
        return -EINVAL;
    }

    // Find a free slot for subscription
    for (int i = 0; i < MESSAGING_MAX_SUBSCRIPTIONS; i++) {
        if (!subscription_list.info[i].is_active) {
            size_t topic_len = strlen((char *)topic) + 1;
            subscription_list.info[i].topic = k_malloc(topic_len);
            if (!subscription_list.info[i].topic) {
                LOG_ERR("Failed to allocate memory for topic");
                return -ENOMEM;
            }

            strcpy((char *)subscription_list.info[i].topic, (char *)topic);
            subscription_list.info[i].module_inst = module_inst;
            subscription_list.info[i].is_active = true;
            subscription_list.subscriptions_number++;
            ocre_increment_resource_count(module_inst, OCRE_RESOURCE_TYPE_MESSAGING);
            LOG_INF("Subscribed to topic: %s, current module_inst: %p", (char *)topic, (void *)module_inst);
            return 0;
        }
    }
    LOG_ERR("No free subscription slots available");
    return -ENOMEM;
}

void ocre_messaging_cleanup_container(wasm_module_inst_t module_inst) {
    if (!messaging_system_initialized || !module_inst) {
        LOG_ERR("Messaging system not initialized or invalid module %p", (void *)module_inst);
        return;
    }
    // Clean up subscriptions
    for (int i = 0; i < MESSAGING_MAX_SUBSCRIPTIONS; i++) {
        if (subscription_list.info[i].is_active && subscription_list.info[i].module_inst == module_inst) {
            k_free(subscription_list.info[i].topic);
            subscription_list.info[i].topic = NULL;
            subscription_list.info[i].module_inst = NULL;
            subscription_list.info[i].is_active = false;
            subscription_list.subscriptions_number--;
            ocre_decrement_resource_count(module_inst, OCRE_RESOURCE_TYPE_MESSAGING);
            LOG_INF("Cleaned up subscription %d for module %p", i, (void *)module_inst);
        }
    }
    LOG_INF("Cleaned up messaging resources for module %p", (void *)module_inst);
}

int ocre_messaging_free_module_event_data(wasm_exec_env_t exec_env, uint32_t topic_offset, uint32_t content_offset,
                                          uint32_t payload_offset) {
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
