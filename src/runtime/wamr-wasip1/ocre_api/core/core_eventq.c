/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "core_external.h"

int core_eventq_init(core_eventq_t *eventq, size_t item_size, size_t max_items)
{
	eventq->buffer = core_malloc(item_size * max_items);
	if (!eventq->buffer) {
		return -ENOMEM;
	}
	eventq->item_size = item_size;
	eventq->max_items = max_items;
	eventq->count = 0;
	eventq->head = 0;
	eventq->tail = 0;
	pthread_mutex_init(&eventq->mutex, NULL);
	pthread_cond_init(&eventq->cond, NULL);
	return 0;
}

int core_eventq_peek(core_eventq_t *eventq, void *event)
{
	pthread_mutex_lock(&eventq->mutex);
	if (eventq->count == 0) {
		pthread_mutex_unlock(&eventq->mutex);
		return -ENOMSG;
	}
	memcpy(event, (char *)eventq->buffer + (eventq->head * eventq->item_size), eventq->item_size);
	pthread_mutex_unlock(&eventq->mutex);
	return 0;
}

int core_eventq_get(core_eventq_t *eventq, void *event)
{
	pthread_mutex_lock(&eventq->mutex);
	if (eventq->count == 0) {
		pthread_mutex_unlock(&eventq->mutex);
		return -ENOENT;
	}
	memcpy(event, (char *)eventq->buffer + (eventq->head * eventq->item_size), eventq->item_size);
	eventq->head = (eventq->head + 1) % eventq->max_items;
	eventq->count--;
	pthread_mutex_unlock(&eventq->mutex);
	return 0;
}

int core_eventq_put(core_eventq_t *eventq, const void *event)
{
	pthread_mutex_lock(&eventq->mutex);
	if (eventq->count >= eventq->max_items) {
		pthread_mutex_unlock(&eventq->mutex);
		return -ENOMEM;
	}
	memcpy((char *)eventq->buffer + (eventq->tail * eventq->item_size), event, eventq->item_size);
	eventq->tail = (eventq->tail + 1) % eventq->max_items;
	eventq->count++;
	pthread_cond_signal(&eventq->cond);
	pthread_mutex_unlock(&eventq->mutex);
	return 0;
}

void core_eventq_destroy(core_eventq_t *eventq)
{
	pthread_mutex_destroy(&eventq->mutex);
	pthread_cond_destroy(&eventq->cond);
	if (eventq->buffer) {
		core_free(eventq->buffer);
		eventq->buffer = NULL;
	}
}
