/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"

#include <errno.h>
#include <stdio.h>

int core_mq_init(core_mq_t *mq, const char *name, size_t msg_size, uint32_t max_msgs)
{
    struct mq_attr attr;

    // Configure the message queue attributes
    attr.mq_flags = 0; // Blocking mode
    attr.mq_maxmsg = max_msgs; // Maximum number of messages in the queue
    attr.mq_msgsize = msg_size; // Size of each message
    attr.mq_curmsgs = 0; // Number of messages currently in the queue

    // Try to unlink the message queue, but ignore error if it doesn't exist
    if (mq_unlink(name) == -1 && errno != ENOENT)
    {
        perror("Main: mq_unlink");
        return -errno;
    }

    // Create the message queue
    mq->msgq = mq_open(name, O_CREAT | O_RDWR, 0644, &attr);
    if (mq->msgq == (mqd_t)-1) {
        perror("Failed to create message queue");
        return -errno;
    }
    return 0;
}

int core_mq_send(core_mq_t *mq, const void *data, size_t msg_len)
{
    int ret = mq_send(mq->msgq, (const char *)data, msg_len, MQ_DEFAULT_PRIO);

    if (ret == -1) {
        perror("Failed to send message");
    }
    return ret;
}

int core_mq_recv(core_mq_t *mq, void *data)
{
    struct mq_attr attr;
    unsigned int priority;

    // Get message queue attributes
    if (mq_getattr(mq->msgq, &attr) == -1) {
        perror("Failed to get message queue attributes");
        return -errno;
    }
    ssize_t bytes_received = mq_receive(mq->msgq, (char *)data, attr.mq_msgsize, &priority);
        if (bytes_received == -1) {
            perror("Failed to receive message");
            return -errno;
        }
    return bytes_received;
}
