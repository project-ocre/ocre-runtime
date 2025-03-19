
#include "wasm_export.h"

/**
 * @typedef ocre_msg
 *
 * Structure of ocre messages
 *
 */
typedef struct ocre_msg {
    // message id - increments on each message
    uint64_t mid;

    // url of the request
    char *topic;

    // payload format (MIME type??)
    char *content_type;

    // payload of the request, currently only support attr_container_t type
    void *payload;

    // length in bytes of the payload
    int payload_len;
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
