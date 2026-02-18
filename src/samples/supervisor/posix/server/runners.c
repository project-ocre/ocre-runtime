/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

#include <ocre/ocre.h>
#include <uthash/utlist.h>
#include <zcbor_encode.h>

#include "../ipc.h"
#include "ocre/container.h"

#define TX_BUFFER_SIZE 32
#define RX_BUFFER_SIZE 32

struct runner {
	pthread_mutex_t mutex;
	int socket;
	bool finished;
	struct ocre_container *container;
	pthread_t run_thread;
	pthread_t socket_thread;
};

static void *run_thread(void *arg)
{
	uint8_t tx_buf[TX_BUFFER_SIZE];
	struct runner *runner = (struct runner *)arg;

	/* Call the actual function */
	int result = ocre_container_start(runner->container);

	pthread_mutex_lock(&runner->mutex);

	if (runner->finished) {
		pthread_mutex_unlock(&runner->mutex);
		return NULL;
	}

	pthread_mutex_unlock(&runner->mutex);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, TX_BUFFER_SIZE, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_START_RSP);
	if (!success) {
		fprintf(stderr, "Encoding response failed\n");
		return NULL;
	}

	success = zcbor_int32_put(enc_state, result);
	if (!success) {
		fprintf(stderr, "Encoding result failed\n");
		return NULL;
	}

	int response_len = enc_state->payload - tx_buf;

	if (send(runner->socket, tx_buf, response_len, 0) < 0) {
		perror("send");
	}

	if (close(runner->socket) < 0) {
		perror("close");
	}

	pthread_mutex_lock(&runner->mutex);

	runner->finished = true;

	pthread_mutex_unlock(&runner->mutex);

	/* Wait for the socket thread to finish */
	pthread_join(runner->socket_thread, NULL);

	return NULL;
}

static void *socket_thread(void *arg)
{
	struct runner *runner = (struct runner *)arg;
	uint8_t rx_buf[RX_BUFFER_SIZE];

	while (true) {
		pthread_mutex_lock(&runner->mutex);

		if (runner->finished) {
			pthread_mutex_unlock(&runner->mutex);
			return NULL;
		};

		pthread_mutex_unlock(&runner->mutex);

		ssize_t n = recv(runner->socket, rx_buf, sizeof(rx_buf), 0);

		if (n <= 0) {
			if (n < 0) {
				perror("recv");
			} else {
				fprintf(stderr, "Client disconnected\n");
			}

			pthread_mutex_lock(&runner->mutex);

			runner->finished = true;

			close(runner->socket);
			pthread_mutex_unlock(&runner->mutex);

			ocre_container_kill(runner->container);

			return NULL;
		}
	}

	return NULL;
}

int container_runner_dispatch(struct ocre_container *container, int socket)
{
	struct runner *runner = NULL;
	pthread_attr_t attr;

	runner = malloc(sizeof(struct runner));
	if (!runner) {
		fprintf(stderr, "Failed to allocate memory for runner\n");
		return -1;
	}

	memset(runner, 0, sizeof(struct runner));

	runner->container = container;
	runner->socket = socket;

	int rc = pthread_mutex_init(&runner->mutex, NULL);
	if (rc) {
		fprintf(stderr, "Failed to initialize mutex: rc=%d", rc);
		goto error_runner;
	}

	rc = pthread_attr_init(&attr);
	if (rc) {
		fprintf(stderr, "Failed to initialize thread attributes: rc=%d", rc);
		goto error_mutex;
	}

	rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (rc) {
		fprintf(stderr, "Failed to initialize thread attributes: rc=%d", rc);
		goto error_attr;
	}

	/* The socket thread is joinable */
	rc = pthread_create(&runner->socket_thread, NULL, socket_thread, runner);
	if (rc) {
		fprintf(stderr, "Failed to create socket thread: rc=%d", rc);
		goto error_mutex;
	}

	/* The run_thread is detached */
	rc = pthread_create(&runner->run_thread, &attr, run_thread, runner);
	if (rc) {
		fprintf(stderr, "Failed to create runner thread: rc=%d", rc);
		goto error_thread;
	}

	rc = pthread_attr_destroy(&attr);
	if (rc) {
		fprintf(stderr, "Failed to destroy thread attributes: rc=%d", rc);
	}

	return 0;

error_thread:
	rc = pthread_mutex_lock(&runner->mutex);
	if (rc) {
		fprintf(stderr, "Failed to lock mutex: rc=%d", rc);
	}

	runner->finished = true;

	rc = pthread_mutex_unlock(&runner->mutex);
	if (rc) {
		fprintf(stderr, "Failed to unlock mutex: rc=%d", rc);
	}

	pthread_join(runner->socket_thread, NULL);

error_attr:
	rc = pthread_attr_destroy(&attr);
	if (rc) {
		fprintf(stderr, "Failed to destroy thread attributes: rc=%d", rc);
	}

error_mutex:
	pthread_mutex_destroy(&runner->mutex);

error_runner:
	free(runner);
	return -1;
}
