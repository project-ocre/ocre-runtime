/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre, a Series of LF Projects, LLC
 *
 * SPDX-License-License: Apache-2.0
 */

#ifndef OCRE_MESSAGING_H
#define OCRE_MESSAGING_H

#include <ocre/ocre.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <wasm_export.h>

#define MESSAGING_QUEUE_SIZE 100

/**
 * @brief Structure representing an OCRE message.
 */
typedef struct {
    uint32_t mid;         ///< Message ID
    void *topic;          ///< Topic of the message (pointer to string)
    void *content_type;   ///< Content type (e.g., MIME type, pointer to string)
    void *payload;        ///< Message payload
    uint32_t payload_len; ///< Length of the payload
} ocre_msg_t;

/**
 * @brief Initialize the OCRE messaging system.
 */
int ocre_messaging_init(void);

/**
 * @brief Publish a message to the specified topic.
 *
 * @param exec_env WASM execution environment.
 * @param topic The name of the topic to publish to (pointer).
 * @param content_type The content type of the message (e.g., MIME type, pointer).
 * @param payload The message payload.
 * @param payload_len The length of the payload.
 * @return 0 on success, negative error code on failure.
 */
int ocre_messaging_publish(wasm_exec_env_t exec_env, void *topic, void *content_type, void *payload, int payload_len);

/**
 * @brief Subscribe to messages on a specified topic.
 *
 * @param exec_env WASM execution environment.
 * @param topic The name of the topic to subscribe to (pointer).
 * @return 0 on success, negative error code on failure.
 */
int ocre_messaging_subscribe(wasm_exec_env_t exec_env, void *topic);

/**
 * @brief Clean up messaging resources for a WASM module.
 *
 * @param module_inst The WASM module instance to clean up.
 */
void ocre_messaging_cleanup_container(wasm_module_inst_t module_inst);

/**
 * @brief Frees allocated memory for a messaging event in the WASM module.
 *
 * This function releases the allocated memory for the topic, content-type, and payload
 * associated with a messaging event received by the WASM module. It should be called
 * after processing the message to prevent memory leaks.
 *
 * @param exec_env        WASM execution environment.
 * @param topic_offset    Offset in WASM memory for the message topic.
 * @param content_offset  Offset in WASM memory for the message content-type.
 * @param payload_offset  Offset in WASM memory for the message payload.
 *
 * @return OCRE_SUCCESS on success, negative error code on failure.
 */
int ocre_messaging_free_module_event_data(wasm_exec_env_t exec_env, uint32_t topic_offset, uint32_t content_offset,
                                           uint32_t payload_offset);

#endif /* OCRE_MESSAGING_H */