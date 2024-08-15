/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_MESSAGES_H_
#define OCRE_MESSAGES_H_

#include <stdint.h>

struct base {};

#include <components/container_supervisor/message_types.h>

struct base_msg {
    int msg_id;
    void *from;
    struct base msg;
};

struct uninstall_msg {
    int msg_id;
    void *from;
    struct uninstall msg;
};

struct install_msg {
    int msg_id;
    void *from;
    struct install msg;
};

struct app_manager_response_msg {
    int msg_id;
    void *from;
    struct app_manager_response msg;
};

struct query_msg {
    int msg_id;
    void *from;
    struct query msg;
};

struct ocre_message {
  uint32_t event;

    union {
        union {
            struct install_msg install_msg;
            struct query_msg query_msg;
            struct uninstall_msg uninstall_msg;
            struct app_manager_response_msg app_manager_response_msg;
            struct base_msg base_msg;
        } iwasm;
    } components;
};

#endif