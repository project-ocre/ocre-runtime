/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#include <ocre/ocre.h>

int process_request(struct ocre_context *ctx, int socket, uint8_t *rx_buf, int rx_len, uint8_t *tx_buf,
		    size_t tx_buf_size);
