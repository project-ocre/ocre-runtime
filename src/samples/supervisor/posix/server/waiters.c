/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#include <sys/socket.h>

#include <ocre/ocre.h>
#include <uthash/utlist.h>
#include <zcbor_encode.h>

#include "../ipc.h"

#define TX_BUFFER_SIZE 2048
#define RX_BUFFER_SIZE 2048

struct client {
	int socket;
	struct client *next;
};

struct waiter {
	struct ocre_container *container;
	pthread_t thread;
	struct client *clients;
	struct waiter *next;
};

static struct waiter *waiters = NULL;

static void *container_wait_thread(void *arg)
{
	uint8_t tx_buf[TX_BUFFER_SIZE];
	struct waiter *waiter = (struct waiter *)arg;

	/* Call the actual function */
	waiter->result = ocre_container_wait(waiter->container, &waiter->exit_status);

	fprintf(stderr, "container wait returned\n");

	pthread_mutex_lock(&mutex);

	LL_DELETE(waiters, waiter);

	pthread_mutex_unlock(&mutex);

	fprintf(stderr, "removed from waiters\n");

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, TX_BUFFER_SIZE, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_WAIT_RSP);
	if (!success) {
		printf("Encoding response failed\n");
		return NULL;
	}

	success = zcbor_int32_put(enc_state, waiter->result);
	if (!success) {
		printf("Encoding result failed\n");
		return NULL;
	}

	if (waiter->result == 0) {
		success = zcbor_int32_put(enc_state, waiter->exit_status);
		if (!success) {
			printf("Encoding exit status failed\n");
			return NULL;
		}
	}

	int response_len = enc_state->payload - tx_buf;

	struct client *client = NULL;
	struct client *elt = NULL;

	LL_FOREACH_SAFE(waiter->clients, client, elt)
	{
		if (send(client->socket, tx_buf, response_len, 0) < 0) {
			perror("send");
		}

		fprintf(stderr, "Response sent from waiter (%d bytes)\n", response_len);

		if (close(client->socket) < 0) {
			perror("close");
		}

		LL_DELETE(waiter->clients, client);
	}

	LL_DELETE(waiters, waiter);

	return NULL;
}

static struct waiter *waiter_get_or_new(struct ocre_container *container)
{
	struct waiter *waiter;

	LL_SEARCH_SCALAR(waiters, waiter, container, container);
	if (waiter) {
		fprintf(stderr, "Waiter already exists for container %p\n", container);
		// pthread_mutex_unlock(&mutex);
		return waiter;
	}

	// pthread_mutex_unlock(&mutex);

	waiter = malloc(sizeof(struct waiter));
	if (!waiter) {
		fprintf(stderr, "Failed to allocate memory for waiter\n");
		return NULL;
	}

	memset(waiter, 0, sizeof(struct waiter));

	waiter->container = container;

	int rc = pthread_create(&waiter->thread, NULL, container_wait_thread, waiter);
	if (rc) {
		fprintf(stderr, "Failed to create thread: rc=%d", rc);
		free(waiter);
		return NULL;
	}

	pthread_mutex_unlock(&waiter->mutex);

	// pthread_mutex_lock(&mutex);

	LL_APPEND(waiters, waiter);

	return waiter;
}

static int waiter_add_client(struct waiter *waiter, int socket)
{
	struct client *client = malloc(sizeof(struct client));
	if (!client) {
		fprintf(stderr, "Failed to allocate memory for client\n");
		return -1;
	}

	memset(client, 0, sizeof(struct client));

	client->socket = socket;

	LL_APPEND(waiter->clients, client);

	return 0;
}

int container_waiter_add_client(struct ocre_container *container, int socket)
{
	int ret = -1;
	pthread_mutex_lock(&mutex);
	struct waiter *waiter = waiter_get_or_new(container);

	if (!waiter) {
		fprintf(stderr, "Failed to get or create waiter for container %p\n", container);
		pthread_mutex_unlock(&mutex);
		return -1;
	}

	ret = waiter_add_client(waiter, socket);

	pthread_mutex_unlock(&mutex);

	return ret;
}
