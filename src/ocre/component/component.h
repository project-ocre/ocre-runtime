/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_COMPONENT_H
#define OCRE_COMPONENT_H

#include <zephyr/kernel.h>
#include <ocre/messages.h>

#define MSG_QUEUE_DEPTH 16

#define COMPONENT_SEND_SIMPLE(c, e)                                            \
  struct ocre_message _msg = {.event = e};                                      \
  ocre_component_send(c, &_msg)

struct ocre_component {
  struct ocre_message msg; /*!< Message struct for reading messages into */
  struct k_msgq msgq;     /*!< Message queue to read from */
  char __aligned(4)
      msgq_buffer[MSG_QUEUE_DEPTH *
                  sizeof(struct ocre_message)]; /*!< Message queue buffer */
};

void ocre_component_init(struct ocre_component *component);

int ocre_component_send(struct ocre_component *component,
                       struct ocre_message *msg);

#endif