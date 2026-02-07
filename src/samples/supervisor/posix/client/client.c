/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zcbor_common.h>

#include <ocre/ocre.h>

#include "../ipc.h"
#include "zcbor_helpers.h"

#define SOCK_PATH	    "ocre.sock"

/* Buffer sizes for IPC communication */
#define SMALL_PAYLOAD_SIZE  64
#define MEDIUM_PAYLOAD_SIZE 256
#define LARGE_PAYLOAD_SIZE  1024
#define STRING_BUFFER_SIZE  256

struct ocre_container {
	char *id;
};

static int make_request(const uint8_t *send_data, size_t send_len, uint8_t *recv_data, size_t recv_len)
{
	int s;
	struct sockaddr_un remote = {
		.sun_family = AF_UNIX,
	};

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return -1;
	}

	strcpy(remote.sun_path, SOCK_PATH);
	size_t len = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(s, (struct sockaddr *)&remote, len) == -1) {
		perror("connect");
		close(s);
		return -1;
	}

	if (send(s, send_data, send_len, 0) == -1) {
		perror("send");
		close(s);
		return -1;
	}

	if ((len = recv(s, recv_data, recv_len, 0)) <= 0) {
		if (len < 0)
			perror("recv");
		else
			printf("Server closed connection\n");
		close(s);
		return -1;
	}

	close(s);

	return len;
}

int ocre_is_valid_id(const char *id)
{
	/* Cannot be NULL */

	if (!id) {
		return 0;
	}

	/* Cannot be empty or start with a dot '.' */

	if (id[0] == '\0' || id[0] == '.') {
		return 0;
	}

	/* Can only contain alphanumeric characters, dots, underscores, and hyphens */

	for (size_t i = 0; i < strlen(id); i++) {
		if ((isalnum(id[i]) && islower(id[i])) || id[i] == '.' || id[i] == '_' || id[i] == '-') {
			continue;
		}

		return 0;
	}

	return 1;
}

/* Stub implementation of ocre_build_configuration */
const struct ocre_config ocre_build_configuration = {
	.version = "stub",
	.commit_id = "stub",
	.build_info = "stub",
	.build_date = "stub",
};

/* Context functions from context.h */

struct ocre_container *ocre_context_create_container(struct ocre_context *context, const char *image,
						     const char *const runtime, const char *container_id, bool detached,
						     const struct ocre_container_args *arguments)
{
	int rc;

	uint8_t payload[LARGE_PAYLOAD_SIZE];
	bool success;

	/* Encode the request */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = zcbor_uint32_put(encoding_state, OCRE_IPC_CONTEXT_CREATE_CONTAINER_REQ);
	if (!success) {
		printf("Encoding request failed: %d\n", zcbor_peek_error(encoding_state));
		return NULL;
	}

	success = encode_string_or_nil(encoding_state, image);
	if (!success) {
		printf("Encoding image failed: %d\n", zcbor_peek_error(encoding_state));
		return NULL;
	}

	success = encode_string_or_nil(encoding_state, runtime);
	if (!success) {
		printf("Encoding runtime failed: %d\n", zcbor_peek_error(encoding_state));
		return NULL;
	}

	success = encode_string_or_nil(encoding_state, container_id);
	if (!success) {
		printf("Encoding container_id failed: %d\n", zcbor_peek_error(encoding_state));
		return NULL;
	}

	success = zcbor_bool_put(encoding_state, detached);
	if (!success) {
		printf("Encoding detached failed: %d\n", zcbor_peek_error(encoding_state));
		return NULL;
	}

	/* Encode container arguments */
	if (arguments == NULL) {
		success = zcbor_nil_put(encoding_state, NULL);
		if (!success) {
			printf("Encoding nil arguments failed: %d\n", zcbor_peek_error(encoding_state));
			return NULL;
		}
	} else {
		/* Encode as a map or list of arrays */
		success = zcbor_list_start_encode(encoding_state, 4);
		if (!success) {
			printf("Encoding list start failed: %d\n", zcbor_peek_error(encoding_state));
			return NULL;
		}

		success = encode_string_array(encoding_state, arguments->argv);
		if (!success) {
			printf("Encoding argv failed: %d\n", zcbor_peek_error(encoding_state));
			return NULL;
		}

		success = encode_string_array(encoding_state, arguments->envp);
		if (!success) {
			printf("Encoding envp failed: %d\n", zcbor_peek_error(encoding_state));
			return NULL;
		}

		success = encode_string_array(encoding_state, arguments->capabilities);
		if (!success) {
			printf("Encoding capabilities failed: %d\n", zcbor_peek_error(encoding_state));
			return NULL;
		}

		success = encode_string_array(encoding_state, arguments->mounts);
		if (!success) {
			printf("Encoding mounts failed: %d\n", zcbor_peek_error(encoding_state));
			return NULL;
		}

		success = zcbor_list_end_encode(encoding_state, 4);
		if (!success) {
			printf("Encoding list end failed: %d\n", zcbor_peek_error(encoding_state));
			return NULL;
		}
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return NULL;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 2, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return NULL;
	}

	if (decoded_rsp != OCRE_IPC_CONTEXT_CREATE_CONTAINER_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return NULL;
	}

	/* Decode the ID string */
	struct zcbor_string id_str;
	success = zcbor_tstr_decode(decoding_state, &id_str);
	if (!success) {
		/* Check if it's nil (NULL was returned) */
		if (zcbor_nil_expect(decoding_state, NULL)) {
			return NULL;
		}

		printf("Decoding ID failed: %d\n", zcbor_peek_error(decoding_state));
		return NULL;
	}

	struct ocre_container *container = malloc(sizeof(struct ocre_container));
	if (!container) {
		printf("Memory allocation failed\n");
		return NULL;
	}

	container->id = malloc(id_str.len + 1);
	if (!container->id) {
		printf("Memory allocation failed\n");
		free(container);
		return NULL;
	}

	memcpy(container->id, id_str.value, id_str.len);

	container->id[id_str.len] = '\0';
	return container;
}

struct ocre_container *ocre_context_get_container_by_id(struct ocre_context *context, const char *id)
{
	(void)context; /* Context is not sent over IPC - server uses global context */
	int rc;

	uint8_t payload[MEDIUM_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server expects only the ID string */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTEXT_GET_CONTAINER_BY_ID_REQ, id);
	if (!success) {
		printf("Encoding request failed: %d\n", zcbor_peek_error(encoding_state));
		return NULL;
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return NULL;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 2, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return NULL;
	}

	if (decoded_rsp != OCRE_IPC_CONTEXT_GET_CONTAINER_BY_ID_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return NULL;
	}

	/* Decode the result (0 = found, -1 = not found) */
	int32_t result;
	success = zcbor_int32_decode(decoding_state, &result);

	if (!success) {
		printf("Decoding result failed: %d\n", zcbor_peek_error(decoding_state));
		return NULL;
	}

	if (result != 0) {
		return NULL; /* Container not found */
	}

	/* Create a local container struct with the ID */
	struct ocre_container *container = malloc(sizeof(struct ocre_container));
	if (!container) {
		printf("Memory allocation failed\n");
		return NULL;
	}

	container->id = strdup(id);
	if (!container->id) {
		printf("Memory allocation failed\n");
		free(container);
		return NULL;
	}

	return container;
}

int ocre_context_remove_container(struct ocre_context *context, struct ocre_container *container)
{
	(void)context; /* Context is not sent over IPC - server uses global context */
	int rc;

	uint8_t payload[MEDIUM_PAYLOAD_SIZE];
	bool success;

	if (!container || !container->id) {
		return -1;
	}

	/* Encode the request - server expects container ID string */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTEXT_REMOVE_CONTAINER_REQ, container->id);
	if (!success) {
		printf("Encoding request failed: %d\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return -1;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 2, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (decoded_rsp != OCRE_IPC_CONTEXT_REMOVE_CONTAINER_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return -1;
	}

	int32_t decoded_status;
	success = zcbor_int32_decode(decoding_state, &decoded_status);

	if (!success) {
		printf("Decoding status failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	return decoded_status;
}

int ocre_context_get_container_count(struct ocre_context *context)
{
	(void)context; /* Context is not sent over IPC - server uses global context */
	int rc;

	uint8_t payload[SMALL_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server takes no parameters */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = zcbor_uint32_put(encoding_state, OCRE_IPC_CONTEXT_GET_CONTAINER_COUNT_REQ);
	if (!success) {
		printf("Encoding failed: %d\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return -1;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 2, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (decoded_rsp != OCRE_IPC_CONTEXT_GET_CONTAINER_COUNT_RSP) {
		printf("Unexpected response.\n");
		return -1;
	}

	int32_t decoded_status;
	success = zcbor_int32_decode(decoding_state, &decoded_status);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	return decoded_status;
}

int ocre_context_get_containers(struct ocre_context *context, struct ocre_container **containers, int max_size)
{
	(void)context; /* Context is not sent over IPC - server uses global context */
	int rc;

	uint8_t payload[SMALL_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server expects only max_size */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = zcbor_uint32_put(encoding_state, OCRE_IPC_CONTEXT_GET_CONTAINERS_REQ);
	if (!success) {
		printf("Encoding req failed: %d\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	success = zcbor_int32_put(encoding_state, max_size);
	if (!success) {
		printf("Encoding size failed: %d\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	uint8_t rx_buf[LARGE_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return -1;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 1, rx_buf, rc, max_size + 2, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (decoded_rsp != OCRE_IPC_CONTEXT_GET_CONTAINERS_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return -1;
	}

	/* Decode the count */
	int32_t count;
	success = zcbor_int32_decode(decoding_state, &count);

	if (!success) {
		printf("Decoding count failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (count < 0) {
		return count; /* Error code */
	}

	/* Decode the container ID strings and create container structs */
	for (int i = 0; i < count && i < max_size; i++) {
		struct zcbor_string id_str;

		/* Try to decode string first */
		if (zcbor_tstr_decode(decoding_state, &id_str)) {
			struct ocre_container *container = malloc(sizeof(struct ocre_container));
			if (!container) {
				printf("Memory allocation failed\n");
				return -1;
			}

			container->id = malloc(id_str.len + 1);
			if (!container->id) {
				printf("Memory allocation failed\n");
				free(container);
				return -1;
			}

			memcpy(container->id, id_str.value, id_str.len);
			container->id[id_str.len] = '\0';
			containers[i] = container;
		} else if (zcbor_nil_expect(decoding_state, NULL)) {
			/* nil means NULL container */
			containers[i] = NULL;
		} else {
			printf("Decoding container ID %d failed: %d\n", i, zcbor_peek_error(decoding_state));
			return -1;
		}
	}

	return count;
}

const char *ocre_context_get_working_directory(const struct ocre_context *context)
{
	(void)context; /* Context is not sent over IPC - server uses global context */
	int rc;
	static char workdir_buffer[STRING_BUFFER_SIZE];

	uint8_t payload[SMALL_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server takes no parameters */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = zcbor_uint32_put(encoding_state, OCRE_IPC_CONTEXT_GET_WORKING_DIRECTORY_REQ);
	if (!success) {
		printf("Encoding failed: %d\n", zcbor_peek_error(encoding_state));
		return NULL;
	}

	uint8_t rx_buf[MEDIUM_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return NULL;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 2, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return NULL;
	}

	if (decoded_rsp != OCRE_IPC_CONTEXT_GET_WORKING_DIRECTORY_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return NULL;
	}

	/* Decode the working directory string */
	struct zcbor_string workdir_str;
	success = zcbor_tstr_decode(decoding_state, &workdir_str);

	if (!success) {
		/* Check if it's nil (NULL was returned) */
		if (zcbor_nil_expect(decoding_state, NULL)) {
			return NULL;
		}
		printf("Decoding workdir failed: %d\n", zcbor_peek_error(decoding_state));
		return NULL;
	}

	/* Copy to static buffer */
	size_t copy_len = workdir_str.len < STRING_BUFFER_SIZE - 1 ? workdir_str.len : STRING_BUFFER_SIZE - 1;
	memcpy(workdir_buffer, workdir_str.value, copy_len);
	workdir_buffer[copy_len] = '\0';

	return workdir_buffer;
}

/* Container functions from container.h */

int ocre_container_start(struct ocre_container *container)
{
	int rc;
	uint8_t tx_payload[SMALL_PAYLOAD_SIZE];
	uint8_t rx_payload[SMALL_PAYLOAD_SIZE];
	bool success;

	if (!container) {
		return -1;
	}

	/* Encode the request */
	ZCBOR_STATE_E(encoding_state, 0, tx_payload, sizeof(tx_payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTAINER_START_REQ, container->id);
	if (!success) {
		printf("Encoding failed: %d\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	rc = make_request(tx_payload, encoding_state->payload - tx_payload, rx_payload, sizeof(rx_payload));
	if (rc <= 0) {
		printf("Make request failed.\n");
		return -1;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_payload, rc, 2, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (decoded_rsp != OCRE_IPC_CONTAINER_START_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return -1;
	}

	int32_t decoded_status;
	success = zcbor_int32_decode(decoding_state, &decoded_status);

	if (!success) {
		printf("Decoding status failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	return decoded_status;
}

ocre_container_status_t ocre_container_get_status(struct ocre_container *container)
{
	int rc;

	if (!container || !container->id) {
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	uint8_t payload[MEDIUM_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server expects container ID string */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTAINER_GET_STATUS_REQ, container->id);
	if (!success) {
		printf("Encoding request failed: %d\n", zcbor_peek_error(encoding_state));
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));
	if (rc <= 0) {
		printf("Make request failed.\n");
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 2, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	if (decoded_rsp != OCRE_IPC_CONTAINER_GET_STATUS_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	int32_t decoded_status;
	success = zcbor_int32_decode(decoding_state, &decoded_status);

	if (!success) {
		printf("Decoding status failed: %d\n", zcbor_peek_error(decoding_state));
		return OCRE_CONTAINER_STATUS_UNKNOWN;
	}

	return (ocre_container_status_t)decoded_status;
}

const char *ocre_container_get_id(const struct ocre_container *container)
{
	return container ? container->id : NULL;
}

const char *ocre_container_get_image(const struct ocre_container *container)
{
	int rc;
	static char image_buffer[STRING_BUFFER_SIZE];

	if (!container || !container->id) {
		return NULL;
	}

	uint8_t payload[MEDIUM_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server expects container ID string */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTAINER_GET_IMAGE_REQ, container->id);
	if (!success) {
		printf("Encoding req failed: %d\n", zcbor_peek_error(encoding_state));
		return NULL;
	}

	uint8_t rx_buf[MEDIUM_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return NULL;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 2, rx_buf, rc, 2, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return NULL;
	}

	if (decoded_rsp != OCRE_IPC_CONTAINER_GET_IMAGE_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return NULL;
	}

	/* Decode the image string */
	struct zcbor_string image_str;
	success = zcbor_tstr_decode(decoding_state, &image_str);

	if (!success) {
		/* Check if it's nil (NULL was returned) */
		if (zcbor_nil_expect(decoding_state, NULL)) {
			return NULL;
		}
		printf("Decoding image failed: %d\n", zcbor_peek_error(decoding_state));
		return NULL;
	}

	/* Copy to static buffer */
	size_t copy_len = image_str.len < STRING_BUFFER_SIZE - 1 ? image_str.len : STRING_BUFFER_SIZE - 1;
	memcpy(image_buffer, image_str.value, copy_len);
	image_buffer[copy_len] = '\0';

	return image_buffer;
}

int ocre_container_pause(struct ocre_container *container)
{
	int rc;

	if (!container || !container->id) {
		return -1;
	}

	uint8_t payload[MEDIUM_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server expects container ID string */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTAINER_PAUSE_REQ, container->id);
	if (!success) {
		printf("Encoding failed: %d\r\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return -1;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 1, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (decoded_rsp != OCRE_IPC_CONTAINER_PAUSE_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return -1;
	}

	int32_t decoded_status;
	success = zcbor_int32_decode(decoding_state, &decoded_status);

	if (!success) {
		printf("Decoding status failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	return decoded_status;
}

int ocre_container_unpause(struct ocre_container *container)
{
	int rc;

	if (!container || !container->id) {
		return -1;
	}

	uint8_t payload[MEDIUM_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server expects container ID string */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTAINER_UNPAUSE_REQ, container->id);
	if (!success) {
		printf("Encoding failed: %d\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return -1;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 1, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (decoded_rsp != OCRE_IPC_CONTAINER_UNPAUSE_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return -1;
	}

	int32_t decoded_status;
	success = zcbor_int32_decode(decoding_state, &decoded_status);

	if (!success) {
		printf("Decoding status failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	return decoded_status;
}

int ocre_container_stop(struct ocre_container *container)
{
	int rc;

	if (!container || !container->id) {
		return -1;
	}

	uint8_t payload[MEDIUM_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server expects container ID string */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTAINER_STOP_REQ, container->id);
	if (!success) {
		printf("Encoding failed: %d\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return -1;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 1, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (decoded_rsp != OCRE_IPC_CONTAINER_STOP_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return -1;
	}

	int32_t decoded_status;
	success = zcbor_int32_decode(decoding_state, &decoded_status);

	if (!success) {
		printf("Decoding status failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	return decoded_status;
}

int ocre_container_kill(struct ocre_container *container)
{
	int rc;

	if (!container || !container->id) {
		return -1;
	}

	uint8_t payload[MEDIUM_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server expects container ID string */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTAINER_KILL_REQ, container->id);
	if (!success) {
		printf("Encoding failed: %d\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return -1;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 1, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (decoded_rsp != OCRE_IPC_CONTAINER_KILL_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return -1;
	}

	int32_t decoded_status;
	success = zcbor_int32_decode(decoding_state, &decoded_status);

	if (!success) {
		printf("Decoding status failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	return decoded_status;
}

int ocre_container_wait(struct ocre_container *container, int *status)
{
	int rc;

	if (!container || !container->id) {
		return -1;
	}

	uint8_t payload[MEDIUM_PAYLOAD_SIZE];
	bool success;

	/* Encode the request - server expects container ID string */
	ZCBOR_STATE_E(encoding_state, 0, payload, sizeof(payload), 0);

	success = encode_request_and_container_id(encoding_state, OCRE_IPC_CONTAINER_WAIT_REQ, container->id);
	if (!success) {
		printf("Encoding failed: %d\n", zcbor_peek_error(encoding_state));
		return -1;
	}

	uint8_t rx_buf[SMALL_PAYLOAD_SIZE];

	rc = make_request(payload, encoding_state->payload - payload, rx_buf, sizeof(rx_buf));

	if (rc <= 0) {
		printf("Make request failed.\n");
		return -1;
	}

	/* Decode the response */
	ZCBOR_STATE_D(decoding_state, 0, rx_buf, rc, 1, 0);

	uint32_t decoded_rsp;
	success = zcbor_uint32_decode(decoding_state, &decoded_rsp);

	if (!success) {
		printf("Decoding failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	if (decoded_rsp != OCRE_IPC_CONTAINER_WAIT_RSP) {
		printf("Unexpected response: %x\n", decoded_rsp);
		return -1;
	}

	int32_t decoded_result;
	success = zcbor_int32_decode(decoding_state, &decoded_result);

	if (!success) {
		printf("Decoding result failed: %d\n", zcbor_peek_error(decoding_state));
		return -1;
	}

	/* Decode the exit status if provided */
	if (status != NULL && decoded_result == 0) {
		int32_t decoded_exit_status;
		success = zcbor_int32_decode(decoding_state, &decoded_exit_status);

		if (!success) {
			printf("Decoding exit status failed: %d\n", zcbor_peek_error(decoding_state));
			return -1;
		}

		*status = decoded_exit_status;
	}

	return decoded_result;
}
