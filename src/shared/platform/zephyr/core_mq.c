/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"
#include <ocre/ocre.h>
LOG_MODULE_DECLARE(platform_mq_component, OCRE_LOG_LEVEL);

int core_mq_init(core_mq_t *mq, const char *name, size_t msg_size, uint32_t max_msgs)
{
    mq->msgq_buffer = k_malloc(msg_size * max_msgs);
    k_msgq_init(&mq->msgq, mq->msgq_buffer, msg_size, max_msgs);
}

int core_mq_send(core_mq_t *mq, const void *data, size_t msg_len)
{
    int ret = k_msgq_put(&mq->msgq, data, MQ_DEFAULT_TIMEOUT);

    if (ret != 0) {
        LOG_HEXDUMP_DBG(data, msg_len, "message");
    }
    return ret;
}

int core_mq_recv(core_mq_t *mq, void *data)
{
    return k_msgq_get(&mq->msgq, data, K_FOREVER);
}
