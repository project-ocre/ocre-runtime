/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONTAINER_MESSAGING_H
#define CONTAINER_MESSAGING_H
#include "wasm_export.h"

/**
 * @typedef ocre_msg
 *
 * Structure of ocre messages
 *
 */
typedef struct ocre_msg {
    // message id - increments on each message
    uint32_t mid;

    // url of the request
    char *topic;

    // payload format (MIME type??)
    char *content_type;

    // payload of the request
    void *payload;

    // length in bytes of the payload
    uint32_t payload_len;
} ocre_msg_t;

/**
 * @brief Initialize OCRE Messaging System.
 *
 */
void ocre_msg_system_init();

/**
 * @brief Publish a message to the specified target.
 *
 * @param topic the name of the topic on which to publish the message
 * @param content_type the content type of the message; it is recommended to use a MIME type
 * @param payload a buffer containing the message contents
 * @param payload_len the length of the payload buffer
 */
int ocre_publish_message(wasm_exec_env_t exec_env, char *topic, char *content_type, void *payload, int payload_len);

/**
 * @brief Subscribe to messages on the specified topic.
 *
 * @param topic the name of the topic on which to subscribe
 * @param handler_name name of callback function that will be called when a message is received on this topic
 */
int ocre_subscribe_message(wasm_exec_env_t exec_env, char *topic, char *handler_name);

/**
 * @brief Register a new WASM module instance
 * @param module_inst WASM module instance to register
 */
void ocre_messaging_register_module(wasm_module_inst_t module_inst);

/**
 * @brief Cleans up all subscriptions associated with a WASM module instance
 * @param module_inst WASM module instance to clean up
 */
void ocre_messaging_cleanup_container(wasm_module_inst_t module_inst);

#endif /* CONTAINER_MESSAGING_H */
