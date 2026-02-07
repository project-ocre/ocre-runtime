#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <zcbor_encode.h>
#include <zcbor_common.h>

bool encode_string_or_nil(zcbor_state_t *state, const char *str);
bool encode_string_array(zcbor_state_t *state, const char **arr);
bool encode_string_or_nil(zcbor_state_t *state, const char *str);
bool encode_string_array(zcbor_state_t *state, const char **arr);
bool encode_request_and_container_id(zcbor_state_t *state, uint32_t req, const char *container_id);

bool decode_string_or_nil(zcbor_state_t *state, char *buffer, size_t buffer_size, bool *is_nil);
bool decode_string_array(zcbor_state_t *state, char **arr, size_t max_count, size_t *out_count, bool *is_nil);
void free_string_array(char **arr, size_t count);
