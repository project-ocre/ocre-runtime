/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <poll.h>

#include <sys/socket.h>

#include <ocre/ocre.h>
#include <uthash/utlist.h>
#include <zcbor_encode.h>

#include "../ipc.h"

#define TX_BUFFER_SIZE 2048

struct client {
	pthread_mutex_t mutex;
	int socket;
	struct client *next;
};

struct waiter {
	pthread_mutex_t mutex;
	struct ocre_container *container;
	pthread_t wait_thread;
	pthread_t socket_thread;
	struct client *clients;
	struct waiter *next;
};

static struct waiter *waiters = NULL;

static void *wait_thread(void *arg)
{
	uint8_t tx_buf[TX_BUFFER_SIZE];
	struct waiter *waiter = (struct waiter *)arg;

	fprintf(stderr, "Wait thread started\n");

	/* Call the actual function */
	int exit_status = 0;
	int result = ocre_container_wait(waiter->container, &exit_status);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, TX_BUFFER_SIZE, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_WAIT_RSP);
	if (!success) {
		printf("Encoding response failed\n");
		return NULL;
	}

	success = zcbor_int32_put(enc_state, result);
	if (!success) {
		printf("Encoding result failed\n");
		return NULL;
	}

	if (result == 0) {
		success = zcbor_int32_put(enc_state, exit_status);
		if (!success) {
			printf("Encoding exit status failed\n");
			return NULL;
		}
	}

	int response_len = enc_state->payload - tx_buf;

	struct client *client = NULL;
	struct client *elt = NULL;

	pthread_mutex_lock(&waiter->mutex);

	LL_FOREACH_SAFE(waiter->clients, client, elt)
	{
		if (send(client->socket, tx_buf, response_len, 0) < 0) {
			perror("send");
		}

		printf("Response sent from waiter (%d bytes)\n", response_len);

		if (close(client->socket) < 0) {
			perror("send");
		}

		LL_DELETE(waiter->clients, client);
	}

	pthread_mutex_unlock(&waiter->mutex);

	fprintf(stderr, "WAIT FOR SOCKET THREAD TO FINISH\n");

	/* Wait for the socket thread to finish */
	pthread_join(waiter->socket_thread, NULL);

	fprintf(stderr, "SOCKET THREAD FINISHED\n");

	LL_DELETE(waiters, waiter);

	fprintf(stderr, "returned\n");

	return NULL;
}

static void *socket_thread(void *arg)
{
	int count;
	struct client *client = NULL;
	struct waiter *waiter = (struct waiter *)arg;

	while (true) {
		pthread_mutex_lock(&waiter->mutex);

		LL_COUNT(waiter->clients, client, count);

		if (count == 0) {
			fprintf(stderr, "No clients connected for polling. Exiting.\n");
			pthread_mutex_unlock(&waiter->mutex);
			break;
		}

		fprintf(stderr, "Number of clients: %d.\n", count);

		// uint8_t tx_buf[TX_BUFFER_SIZE];

		struct pollfd *pfds = malloc(sizeof(struct pollfd) * count);
		if (!pfds) {
			perror("malloc");
			return NULL;
		}

		int i = 0;
		LL_FOREACH(waiter->clients, client)
		{
			pfds[i].fd = client->socket;
			pfds[i].events = POLLIN;
			i++;
		}

		pthread_mutex_unlock(&waiter->mutex);

		int ret = poll(pfds, count, 2500);
		if (ret < 0) {
			perror("poll");
			return NULL;
		}

		for (i = 0; i < count; i++) {
			if (pfds[i].revents & POLLIN) {
				fprintf(stderr, "poll in from %d\n", pfds[i].fd);
				// if (recv(pfds[i].fd, rx_buf, RX_BUFFER_SIZE, 0) < 0) {
				// 	perror("recv");
				// }
			}
		}

		free(pfds);
	}

	return NULL;
}

struct waiter *waiter_get_or_new(struct ocre_container *container)
{
	struct waiter *waiter;

	LL_SEARCH_SCALAR(waiters, waiter, container, container);
	if (waiter) {
		fprintf(stderr, "Waiter already exists for container %p\n", container);
		return waiter;
	}

	waiter = malloc(sizeof(struct waiter));
	if (!waiter) {
		fprintf(stderr, "Failed to allocate memory for waiter\n");
		return NULL;
	}

	memset(waiter, 0, sizeof(struct waiter));

	waiter->container = container;

	int rc = pthread_mutex_init(&waiter->mutex, NULL);
	if (rc) {
		fprintf(stderr, "Failed to initialize mutex: rc=%d", rc);
		free(waiter);
		return NULL;
	}

	rc = pthread_create(&waiter->socket_thread, NULL, socket_thread, waiter);
	if (rc) {
		fprintf(stderr, "Failed to create socket thread: rc=%d", rc);
		free(waiter);
		return NULL;
	}

	rc = pthread_create(&waiter->wait_thread, NULL, wait_thread, waiter);
	if (rc) {
		fprintf(stderr, "Failed to create waiter thread: rc=%d", rc);
		free(waiter);
		return NULL;
	}

	LL_APPEND(waiters, waiter);

	return waiter;
}

int waiter_add_client(struct waiter *waiter, int socket)
{
	struct client *client = malloc(sizeof(struct client));
	if (!client) {
		fprintf(stderr, "Failed to allocate memory for client\n");
		return -1;
	}

	memset(client, 0, sizeof(struct client));

	client->socket = socket;

	pthread_mutex_lock(&waiter->mutex);

	LL_APPEND(waiter->clients, client);

	pthread_mutex_unlock(&waiter->mutex);

	return 0;
}
