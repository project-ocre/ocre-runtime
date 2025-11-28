/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre, a Series of LF Projects, LLC
 *
 * SPDX-License-License: Apache-2.0
 */

#include <ocre/ocre.h>
#include "ocre_core_external.h"
#include <ocre/api/ocre_common.h>
#include <ocre/ocre_messaging/ocre_messaging.h>
#include <string.h>

LOG_MODULE_DECLARE(ocre_cs_component, OCRE_LOG_LEVEL);

#ifndef CONFIG_MESSAGING_MAX_SUBSCRIPTIONS
#define CONFIG_MESSAGING_MAX_SUBSCRIPTIONS 10
#endif
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
	core_mutex_t mutex;
} ocre_messaging_system_t;

static ocre_messaging_system_t messaging_system = {0};
static bool messaging_system_initialized = false;

/* Initialize messaging system */
int ocre_messaging_init(void)
{
	if (messaging_system_initialized) {
		LOG_INF("Messaging system already initialized");
		return 0;
	}

	if (!common_initialized && ocre_common_init() != 0) {
		LOG_ERR("Failed to initialize common subsystem");
		return -EAGAIN;
	}

	memset(&messaging_system, 0, sizeof(ocre_messaging_system_t));

	core_mutex_init(&messaging_system.mutex);

	ocre_register_cleanup_handler(OCRE_RESOURCE_TYPE_MESSAGING, ocre_messaging_cleanup_container);
	messaging_system_initialized = true;
	LOG_INF("Messaging system initialized");
	return 0;
}

/* Cleanup messaging resources for a module */
void ocre_messaging_cleanup_container(wasm_module_inst_t module_inst)
{
	if (!messaging_system_initialized || !module_inst) {
		return;
	}

	core_mutex_lock(&messaging_system.mutex);

	for (int i = 0; i < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS; i++) {
		if (messaging_system.subscriptions[i].is_active &&
		    messaging_system.subscriptions[i].module_inst == module_inst) {
			messaging_system.subscriptions[i].is_active = false;
			messaging_system.subscriptions[i].module_inst = NULL;
			messaging_system.subscriptions[i].topic[0] = '\0';
			messaging_system.subscription_count--;
			ocre_decrement_resource_count(module_inst, OCRE_RESOURCE_TYPE_MESSAGING);
			LOG_DBG("Cleaned up subscription %d for module %p", i, (void *)module_inst);
		}
	}

	core_mutex_unlock(&messaging_system.mutex);

	LOG_DBG("Cleaned up messaging resources for module %p", (void *)module_inst);
}

/* Subscribe to a topic */
int ocre_messaging_subscribe(wasm_exec_env_t exec_env, void *topic)
{
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

	core_mutex_lock(&messaging_system.mutex);

	// Check if already subscribed
	for (int i = 0; i < CONFIG_MESSAGING_MAX_SUBSCRIPTIONS; i++) {
		if (messaging_system.subscriptions[i].is_active &&
		    messaging_system.subscriptions[i].module_inst == module_inst &&
		    strcmp(messaging_system.subscriptions[i].topic, (char *)topic) == 0) {
			LOG_INF("Already subscribed to topic: %s", (char *)topic);
			core_mutex_unlock(&messaging_system.mutex);
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
			core_mutex_unlock(&messaging_system.mutex);
			return 0;
		}
	}

	core_mutex_unlock(&messaging_system.mutex);

	LOG_ERR("No free subscription slots available");
	return -ENOMEM;
}

/* Publish a message */
int ocre_messaging_publish(wasm_exec_env_t exec_env, void *topic, void *content_type, void *payload, int payload_len)
{
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

	core_mutex_lock(&messaging_system.mutex);

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

		core_spinlock_key_t key = core_spinlock_lock(&ocre_event_queue_lock);
		if (core_eventq_put(&ocre_event_queue, &event) != 0) {
			LOG_ERR("Failed to queue messaging event for message ID %d", message_id);
			wasm_runtime_module_free(target_module, topic_offset);
			wasm_runtime_module_free(target_module, content_offset);
			wasm_runtime_module_free(target_module, payload_offset);
		} else {
			message_sent = true;
			LOG_DBG("Queued messaging event for message ID %d", message_id);
		}
		core_spinlock_unlock(&ocre_event_queue_lock, key);
	}

	core_mutex_unlock(&messaging_system.mutex);

	if (message_sent) {
		LOG_DBG("Published message: ID=%d, topic=%s, content_type=%s, payload_len=%d", message_id,
			(char *)topic, (char *)content_type, payload_len);
		message_id++;
		return 0;
	} else {
		LOG_WRN("No matching subscriptions found for topic %s", (char *)topic);
		return -ENOENT;
	}
}

/* Free module event data */
int ocre_messaging_free_module_event_data(wasm_exec_env_t exec_env, uint32_t topic_offset, uint32_t content_offset,
					  uint32_t payload_offset)
{
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
