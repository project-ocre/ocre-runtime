/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>

#include "ocre_core_external.h"
#include "component.h"

void ocre_component_init(struct ocre_component *component) {
    core_mq_init(&component->msgq, "/ocre_component_msgq", sizeof(struct ocre_message), MSG_QUEUE_DEPTH);
}

int ocre_component_send(struct ocre_component *component, struct ocre_message *msg) {
   int ret = core_mq_send(&component->msgq, msg, sizeof(struct ocre_message));

    return ret;
}
