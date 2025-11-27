/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"
#include <string.h>
#include <errno.h>

int core_eventq_init(core_eventq_t *eventq, size_t item_size, size_t max_items)
{
	eventq->buffer = core_malloc(item_size * max_items);
	if (!eventq->buffer) {
		return -ENOMEM;
	}
	eventq->item_size = item_size;
	eventq->max_items = max_items;

	k_msgq_init(&eventq->msgq, (char *)eventq->buffer, item_size, max_items);
	return 0;
}

int core_eventq_peek(core_eventq_t *eventq, void *event)
{
	int ret = k_msgq_peek(&eventq->msgq, event);
	if (ret == 0) {
		return 0;
	} else if (ret == -ENOMSG) {
		return -ENOMSG;
	} else {
		return ret;
	}
}

int core_eventq_get(core_eventq_t *eventq, void *event)
{
	int ret = k_msgq_get(&eventq->msgq, event, K_NO_WAIT);
	if (ret == 0) {
		return 0;
	} else if (ret == -ENOMSG) {
		return -ENOENT;
	} else {
		return ret;
	}
}

int core_eventq_put(core_eventq_t *eventq, const void *event)
{
	int ret = k_msgq_put(&eventq->msgq, event, K_NO_WAIT);
	if (ret == 0) {
		return 0;
	} else if (ret == -ENOMSG) {
		return -ENOMEM;
	} else {
		return ret;
	}
}

void core_eventq_destroy(core_eventq_t *eventq)
{
	if (eventq->buffer) {
		core_free(eventq->buffer);
		eventq->buffer = NULL;
	}
}
