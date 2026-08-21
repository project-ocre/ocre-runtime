/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_HTTP_CLIENT_H
#define OCRE_HTTP_CLIENT_H

#include <wasm_export.h>

/**
 * Outbound HTTP client for WASM containers -- the counterpart to
 * ocre_http.h's inbound request/response bridge. Lets container code issue
 * its own HTTP GET requests (e.g. to poll a status endpoint) rather than
 * only ever answering requests the native side hands it.
 */

/** Blocking HTTP GET to http://host:port/path. Copies the response body into
 * buf, NUL-terminated (truncated if it doesn't fit). Returns the copied body
 * length on a 200 response, or a negative value on any connection, timeout,
 * or non-200 failure. Runs on the calling WASM instance's own thread -- like
 * any blocking native call, it stalls that instance's loop()/onRequest()
 * dispatch for the duration of the request. */
int ocre_http_client_get_wasm(wasm_exec_env_t exec_env, const char *host, int32_t port, const char *path, char *buf,
			      uint32_t buf_len);

#endif /* OCRE_HTTP_CLIENT_H */
