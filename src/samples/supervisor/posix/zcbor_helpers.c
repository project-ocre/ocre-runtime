#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <zcbor_encode.h>
#include <zcbor_decode.h>
#include <zcbor_common.h>

#define STRING_BUFFER_SIZE 256

/* Helper to encode a string or nil if NULL */
bool encode_string_or_nil(zcbor_state_t *state, const char *str)
{
	if (str == NULL) {
		return zcbor_nil_put(state, NULL);
	}
	return zcbor_tstr_put_term(state, str, STRING_BUFFER_SIZE);
}

/* Helper to encode a string array (NULL-terminated) */
bool encode_string_array(zcbor_state_t *state, const char **arr)
{
	if (arr == NULL) {
		return zcbor_nil_put(state, NULL);
	}

	/* Count the strings */
	size_t count = 0;
	while (arr[count] != NULL) {
		count++;
	}

	/* Encode as a CBOR array */
	if (!zcbor_list_start_encode(state, count + 1)) {
		return false;
	}

	for (size_t i = 0; i < count; i++) {
		if (!zcbor_tstr_put_term(state, arr[i], STRING_BUFFER_SIZE)) {
			return false;
		}
	}

	return zcbor_list_end_encode(state, count);
}

bool encode_request_and_container_id(zcbor_state_t *state, uint32_t req, const char *container_id)
{
	bool success = zcbor_uint32_put(state, req);
	if (!success) {
		fprintf(stderr, "Encoding req failed: %d\n", zcbor_peek_error(state));
		return false;
	}

	success = encode_string_or_nil(state, container_id);
	if (!success) {
		fprintf(stderr, "Encoding container id failed: %d\n", zcbor_peek_error(state));
		return false;
	}

	return true;
}

/* Helper to decode a string or nil, returns true if string was decoded, false if nil */
bool decode_string_or_nil(zcbor_state_t *state, char *buffer, size_t buffer_size, bool *is_nil)
{
	*is_nil = false;

	/* Try to decode nil first */
	if (zcbor_nil_expect(state, NULL)) {
		*is_nil = true;
		buffer[0] = '\0';
		return true;
	}

	zcbor_pop_error(state);

	/* Decode as string */
	struct zcbor_string str;
	if (!zcbor_tstr_decode(state, &str)) {
		return false;
	}

	size_t copy_len = str.len < buffer_size - 1 ? str.len : buffer_size - 1;
	memcpy(buffer, str.value, copy_len);
	buffer[copy_len] = '\0';

	return true;
}

/* Helper to decode a string array (CBOR list of strings) */
bool decode_string_array(zcbor_state_t *state, char **arr, size_t max_count, size_t *out_count, bool *is_nil)
{
	*is_nil = false;
	*out_count = 0;

	/* Try to decode nil first */
	if (zcbor_nil_expect(state, NULL)) {
		*is_nil = true;
		return true;
	}

	zcbor_pop_error(state);

	/* Decode as list */
	if (!zcbor_list_start_decode(state)) {
		return false;
	}

	size_t count = 0;
	while (!zcbor_list_end_decode(state) && count < max_count) {
		struct zcbor_string str;

		if (zcbor_nil_expect(state, NULL)) {
			break;
		}

		zcbor_pop_error(state);

		if (!zcbor_tstr_decode(state, &str)) {
			break;
		}

		/* Allocate and copy string */
		arr[count] = malloc(str.len + 1);
		if (arr[count] == NULL) {
			return false;
		}
		memcpy(arr[count], str.value, str.len);
		arr[count][str.len] = '\0';
		count++;
	}

	arr[count] = NULL; /* NULL-terminate the array */
	*out_count = count;

	return true;
}

/* Helper to free a string array */
void free_string_array(char **arr, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		free(arr[i]);
	}
}
