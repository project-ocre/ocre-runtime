/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zcbor_common.h>

#include <ocre/ocre.h>

#include "ipc.h"
#include "zcbor_helpers.h"

#define SOCK_PATH	   "ocre.sock"
#define RX_BUFFER_SIZE	   2048
#define TX_BUFFER_SIZE	   2048
#define STRING_BUFFER_SIZE 256
#define MAX_CONTAINERS	   64
#define MAX_STRING_ARRAY   32

/* Global ocre context for the server */
static struct ocre_context *ctx = NULL;

// static void print_hex(const char *label, const uint8_t *data, size_t len)
// {
// 	printf("HEX %s: ", label);
// 	for (size_t i = 0; i < len; i++) {
// 		printf("%02x ", data[i]);
// 	}
// 	printf("\n");
// }

static int handle_context_create_container(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char image[STRING_BUFFER_SIZE];
	char runtime[STRING_BUFFER_SIZE];
	char container_name[STRING_BUFFER_SIZE];
	bool detached;
	bool image_nil, runtime_nil, container_id_nil, args_nil;

	/* Decode image */
	if (!decode_string_or_nil(dec_state, image, sizeof(image), &image_nil)) {
		printf("Failed to decode image\n");
		printf("decoding failed: %d\r\n", zcbor_peek_error(dec_state));

		return -1;
	}

	/* Decode runtime */
	if (!decode_string_or_nil(dec_state, runtime, sizeof(runtime), &runtime_nil)) {
		printf("Failed to decode runtime\n");
		return -1;
	}

	/* Decode container_id */
	if (!decode_string_or_nil(dec_state, container_name, sizeof(container_name), &container_id_nil)) {
		printf("Failed to decode container_id\n");
		return -1;
	}

	/* Decode detached flag */
	if (!zcbor_bool_decode(dec_state, &detached)) {
		printf("Failed to decode detached\n");
		return -1;
	}

	/* Decode arguments */
	struct ocre_container_args args = {0};
	char *argv_arr[MAX_STRING_ARRAY] = {0};
	char *envp_arr[MAX_STRING_ARRAY] = {0};
	char *caps_arr[MAX_STRING_ARRAY] = {0};
	char *mounts_arr[MAX_STRING_ARRAY] = {0};
	size_t argv_count = 0, envp_count = 0, caps_count = 0, mounts_count = 0;
	bool argv_nil, envp_nil, caps_nil, mounts_nil;

	/* Check if arguments is nil */
	if (zcbor_nil_expect(dec_state, NULL)) {
		args_nil = true;
	} else {
		zcbor_pop_error(dec_state);
		args_nil = false;

		/* Decode as list of 4 arrays */
		if (!zcbor_list_start_decode(dec_state)) {
			printf("Failed to decode arguments list\n");
			return -1;
		}

		/* Decode argv */
		if (!decode_string_array(dec_state, argv_arr, MAX_STRING_ARRAY - 1, &argv_count, &argv_nil)) {
			printf("Failed to decode argv\n");
			return -1;
		}

		/* Decode envp */
		if (!decode_string_array(dec_state, envp_arr, MAX_STRING_ARRAY - 1, &envp_count, &envp_nil)) {
			printf("Failed to decode envp\n");
			free_string_array(argv_arr, argv_count);
			return -1;
		}

		/* Decode capabilities */
		if (!decode_string_array(dec_state, caps_arr, MAX_STRING_ARRAY - 1, &caps_count, &caps_nil)) {
			printf("Failed to decode capabilities\n");
			free_string_array(argv_arr, argv_count);
			free_string_array(envp_arr, envp_count);
			return -1;
		}

		/* Decode mounts */
		if (!decode_string_array(dec_state, mounts_arr, MAX_STRING_ARRAY - 1, &mounts_count, &mounts_nil)) {
			printf("Failed to decode mounts\n");
			free_string_array(argv_arr, argv_count);
			free_string_array(envp_arr, envp_count);
			free_string_array(caps_arr, caps_count);
			return -1;
		}

		zcbor_list_end_decode(dec_state);

		args.argv = argv_nil ? NULL : (const char **)argv_arr;
		args.envp = envp_nil ? NULL : (const char **)envp_arr;
		args.capabilities = caps_nil ? NULL : (const char **)caps_arr;
		args.mounts = mounts_nil ? NULL : (const char **)mounts_arr;
	}

	/* Call the actual function */
	struct ocre_container *container = ocre_context_create_container(
		ctx, image_nil ? NULL : image, runtime_nil ? NULL : runtime, container_id_nil ? NULL : container_name,
		detached, args_nil ? NULL : &args);

	/* Free allocated string arrays */
	if (!args_nil) {
		free_string_array(argv_arr, argv_count);
		free_string_array(envp_arr, envp_count);
		free_string_array(caps_arr, caps_count);
		free_string_array(mounts_arr, mounts_count);
	}

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTEXT_CREATE_CONTAINER_RSP);

	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	const char *container_id = ocre_container_get_id(container);
	if (!container_id) {
		printf("Failed to get container ID\n");
		return -1;
	}

	success = encode_string_or_nil(enc_state, container_id);
	if (!success) {
		printf("Encoding container ID failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_context_get_container_by_id(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	/* Call the actual function */
	struct ocre_container *container = ocre_context_get_container_by_id(ctx, is_nil ? NULL : id);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTEXT_GET_CONTAINER_BY_ID_RSP);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	success = zcbor_int32_put(enc_state, container ? 0 : -1);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_context_remove_container(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	struct ocre_container *container = ocre_context_get_container_by_id(ctx, id);

	if (!container) {
		printf("Container not found\n");
		return -1;
	}

	/* Call the actual function */
	int result = ocre_context_remove_container(ctx, container);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTEXT_REMOVE_CONTAINER_RSP);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	success = zcbor_int32_put(enc_state, result);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_context_get_container_count(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	/* Call the actual function */
	int result = ocre_context_get_container_count(ctx);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTEXT_GET_CONTAINER_COUNT_RSP);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	success = zcbor_int32_put(enc_state, result);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_context_get_containers(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	int32_t max_size;

	/* Decode max_size */
	if (!zcbor_int32_decode(dec_state, &max_size)) {
		printf("Failed to decode max_size\n");
		return -1;
	}

	/* Allocate container array */
	int actual_max = max_size < MAX_CONTAINERS ? max_size : MAX_CONTAINERS;
	struct ocre_container *containers[MAX_CONTAINERS];

	/* Call the actual function */
	int count = ocre_context_get_containers(ctx, containers, actual_max);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTEXT_GET_CONTAINERS_RSP);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	success = zcbor_int32_put(enc_state, count);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	/* Encode container handles if count > 0 */
	if (success && count > 0) {
		for (int i = 0; i < count && success; i++) {
			const char *id = ocre_container_get_id(containers[i]);
			success = encode_string_or_nil(enc_state, id);
		}
	}

	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_context_get_working_directory(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	/* Call the actual function */
	const char *workdir = ocre_context_get_working_directory(ctx);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTEXT_GET_WORKING_DIRECTORY_RSP);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	success = encode_string_or_nil(enc_state, workdir);

	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_container_start(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	struct ocre_container *container = ocre_context_get_container_by_id(ctx, id);

	if (!container) {
		printf("Container not found\n");
		return -1;
	}

	/* Call the actual function */
	int result = ocre_container_start(container);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_START_RSP);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	success = zcbor_int32_put(enc_state, result);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_container_get_status(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	struct ocre_container *container = ocre_context_get_container_by_id(ctx, id);

	if (!container) {
		printf("Container not found\n");
		return -1;
	}

	/* Call the actual function */
	ocre_container_status_t status = ocre_container_get_status(container);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_GET_STATUS_RSP);
	if (success) {
		success = zcbor_int32_put(enc_state, (int32_t)status);
	}

	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_container_get_image(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	struct ocre_container *container = ocre_context_get_container_by_id(ctx, id);

	if (!container) {
		printf("Container not found\n");
		return -1;
	}

	/* Call the actual function */
	const char *image = ocre_container_get_image(container);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_GET_IMAGE_RSP);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	success = encode_string_or_nil(enc_state, image);
	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_container_pause(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	struct ocre_container *container = ocre_context_get_container_by_id(ctx, id);

	if (!container) {
		printf("Container not found\n");
		return -1;
	}

	/* Call the actual function */
	int result = ocre_container_pause(container);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_PAUSE_RSP);
	if (success) {
		success = zcbor_int32_put(enc_state, result);
	}

	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_container_unpause(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	struct ocre_container *container = ocre_context_get_container_by_id(ctx, id);

	if (!container) {
		printf("Container not found\n");
		return -1;
	}

	/* Call the actual function */
	int result = ocre_container_unpause(container);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_UNPAUSE_RSP);
	if (success) {
		success = zcbor_int32_put(enc_state, result);
	}

	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_container_stop(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	struct ocre_container *container = ocre_context_get_container_by_id(ctx, id);

	if (!container) {
		printf("Container not found\n");
		return -1;
	}

	/* Call the actual function */
	int result = ocre_container_stop(container);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_STOP_RSP);
	if (success) {
		success = zcbor_int32_put(enc_state, result);
	}

	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_container_kill(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	struct ocre_container *container = ocre_context_get_container_by_id(ctx, id);

	if (!container) {
		printf("Container not found\n");
		return -1;
	}

	/* Call the actual function */
	int result = ocre_container_kill(container);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_KILL_RSP);
	if (success) {
		success = zcbor_int32_put(enc_state, result);
	}

	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

static int handle_container_wait(zcbor_state_t *dec_state, uint8_t *tx_buf, size_t tx_buf_size)
{
	char id[STRING_BUFFER_SIZE];
	bool is_nil;

	/* Decode id */
	if (!decode_string_or_nil(dec_state, id, sizeof(id), &is_nil)) {
		printf("Failed to decode id\n");
		return -1;
	}

	struct ocre_container *container = ocre_context_get_container_by_id(ctx, id);

	if (!container) {
		printf("Container not found\n");
		return -1;
	}

	/* Call the actual function */
	int exit_status = 0;
	int result = ocre_container_wait(container, &exit_status);

	/* Encode response */
	ZCBOR_STATE_E(enc_state, 0, tx_buf, tx_buf_size, 0);

	bool success = zcbor_uint32_put(enc_state, OCRE_IPC_CONTAINER_WAIT_RSP);
	if (success) {
		success = zcbor_int32_put(enc_state, result);
	}
	if (success && result == 0) {
		success = zcbor_int32_put(enc_state, exit_status);
	}

	if (!success) {
		printf("Encoding response failed\n");
		return -1;
	}

	return enc_state->payload - tx_buf;
}

/* Process a single IPC request */
static int process_request(uint8_t *rx_buf, int rx_len, uint8_t *tx_buf, size_t tx_buf_size)
{
	bool success;

	/* Create zcbor state variable for decoding */
	ZCBOR_STATE_D(decoding_state, 2, rx_buf, rx_len, 10, 0);

	uint32_t decoded_cmd;

	/* Decode the command */
	success = zcbor_uint32_decode(decoding_state, &decoded_cmd);

	if (!success) {
		printf("Decoding command failed: %d\r\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	printf("Decoded CMD: 0x%x\n", decoded_cmd);

	int response_len = -1;

	switch (decoded_cmd) {
		case OCRE_IPC_CONTEXT_CREATE_CONTAINER_REQ:
			response_len = handle_context_create_container(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTEXT_GET_CONTAINER_BY_ID_REQ:
			response_len = handle_context_get_container_by_id(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTEXT_REMOVE_CONTAINER_REQ:
			response_len = handle_context_remove_container(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTEXT_GET_CONTAINER_COUNT_REQ:
			response_len = handle_context_get_container_count(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTEXT_GET_CONTAINERS_REQ:
			response_len = handle_context_get_containers(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTEXT_GET_WORKING_DIRECTORY_REQ:
			response_len = handle_context_get_working_directory(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTAINER_START_REQ:
			response_len = handle_container_start(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTAINER_GET_STATUS_REQ:
			response_len = handle_container_get_status(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTAINER_GET_IMAGE_REQ:
			response_len = handle_container_get_image(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTAINER_PAUSE_REQ:
			response_len = handle_container_pause(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTAINER_UNPAUSE_REQ:
			response_len = handle_container_unpause(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTAINER_STOP_REQ:
			response_len = handle_container_stop(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTAINER_KILL_REQ:
			response_len = handle_container_kill(decoding_state, tx_buf, tx_buf_size);
			break;

		case OCRE_IPC_CONTAINER_WAIT_REQ:
			response_len = handle_container_wait(decoding_state, tx_buf, tx_buf_size);
			break;

		default:
			printf("Unknown command: 0x%x\n", decoded_cmd);
			break;
	}

	return response_len;
}

int main(void)
{

	int rc = ocre_initialize(NULL);
	if (rc) {
		fprintf(stderr, "Failed to initialize runtimes\n");
		return 1;
	}

	ctx = ocre_create_context(NULL);
	if (!ctx) {
		fprintf(stderr, "Failed to create ocre context\n");
		return 1;
	}

	int s, s2;
	struct sockaddr_un remote, local = {
					   .sun_family = AF_UNIX,
				   };
	uint8_t rx_buf[RX_BUFFER_SIZE];
	uint8_t tx_buf[TX_BUFFER_SIZE];

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	strcpy(local.sun_path, SOCK_PATH);
	unlink(local.sun_path);
	size_t len = strlen(local.sun_path) + sizeof(local.sun_family);
	if (bind(s, (struct sockaddr *)&local, len) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(s, 5) == -1) {
		perror("listen");
		exit(1);
	}

	printf("Ocre daemon started, listening on %s\n", SOCK_PATH);

	for (;;) {
		int n;
		printf("Waiting for a connection...\n");
		socklen_t slen = sizeof(remote);
		if ((s2 = accept(s, (struct sockaddr *)&remote, &slen)) == -1) {
			perror("accept");
			exit(1);
		}

		printf("Connected.\n");

		do {
			n = recv(s2, rx_buf, sizeof(rx_buf), 0);
			if (n <= 0) {
				if (n < 0)
					perror("recv");
				else
					printf("Client disconnected\n");
				break;
			}

			// printf("Received %d bytes\n", n);
			// print_hex("received", rx_buf, n);

			/* Process the request and generate response */
			int response_len = process_request(rx_buf, n, tx_buf, sizeof(tx_buf));

			if (response_len < 0) {
				printf("Failed to process request\n");
				break;
			}

			// print_hex("sending", tx_buf, response_len);

			if (send(s2, tx_buf, response_len, 0) < 0) {
				perror("send");
				break;
			}

			printf("Response sent (%d bytes)\n", response_len);

		} while (1);

		close(s2);
	}

	return 0;
}
