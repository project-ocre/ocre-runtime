/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre, a Series of LF Projects, LLC
 *
 * SPDX-License-License: Apache-2.0
 */

#ifndef OCRE_MESSAGING_H
#define OCRE_MESSAGING_H

#include <ocre/ocre.h>
#include "ocre_core_external.h"
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
 * @brief Initialize the messaging system.
 *
 * @return 0 on success, negative error code on failure.
 */
int ocre_messaging_init(void);

/**
 * @brief Subscribe to a topic.
 *
 * @param exec_env WASM execution environment.
 * @param topic Topic to subscribe to.
 * @return 0 on success, negative error code on failure.
 */
int ocre_messaging_subscribe(wasm_exec_env_t exec_env, void *topic);

/**
 * @brief Publish a message to a topic.
 *
 * @param exec_env WASM execution environment.
 * @param topic Topic to publish to.
 * @param content_type Content type of the message.
 * @param payload Message payload.
 * @param payload_len Length of the payload.
 * @return 0 on success, negative error code on failure.
 */
int ocre_messaging_publish(wasm_exec_env_t exec_env, void *topic, void *content_type, void *payload, int payload_len);

/**
 * @brief Free module event data.
 *
 * @param exec_env WASM execution environment.
 * @param topic_offset Topic offset in WASM memory.
 * @param content_offset Content type offset in WASM memory.
 * @param payload_offset Payload offset in WASM memory.
 * @return 0 on success, negative error code on failure.
 */
int ocre_messaging_free_module_event_data(wasm_exec_env_t exec_env, uint32_t topic_offset, uint32_t content_offset, uint32_t payload_offset);

/**
 * @brief Cleanup messaging resources for a module.
 *
 * @param module_inst WASM module instance.
 */
void ocre_messaging_cleanup_container(wasm_module_inst_t module_inst);

#endif /* OCRE_MESSAGING_H */
