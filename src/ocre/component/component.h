/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_COMPONENT_H
#define OCRE_COMPONENT_H

#include "ocre_core_external.h"
#include <messaging/messages.g>

// can't be higher than /proc/sys/fs/mqueue/msg_max which is typically 10
#define MSG_QUEUE_DEPTH 10

#define COMPONENT_SEND_SIMPLE(c, e)                                                                                    \
	struct ocre_message _msg = {.event = e};                                                                       \
	ocre_component_send(c, &_msg)

struct ocre_component {
	struct ocre_message msg; /*!< Message struct for reading messages into */
	core_mq_t msgq;		 /*!< Message queue to read from */
};

void ocre_component_init(struct ocre_component *component);

int ocre_component_send(struct ocre_component *component, struct ocre_message *msg);

#endif // OCRE_COMPONENT_H
