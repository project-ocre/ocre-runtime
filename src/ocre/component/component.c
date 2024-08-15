/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(device_manager_component, OCRE_LOG_LEVEL);

#include "component.h"

void ocre_component_init(struct ocre_component *component) {
    k_msgq_init(&component->msgq, component->msgq_buffer, sizeof(struct ocre_message), MSG_QUEUE_DEPTH);
}

int ocre_component_send(struct ocre_component *component, struct ocre_message *msg) {
    int ret = k_msgq_put(&component->msgq, msg, K_NO_WAIT);
    if (ret != 0) {

        LOG_HEXDUMP_DBG(msg, sizeof(struct ocre_message), "message");
    }

    return ret;
}