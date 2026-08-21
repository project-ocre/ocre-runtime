/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_HTTP_H
#define OCRE_HTTP_H

#include <wasm_export.h>

/**
 * Generic HTTP request/response bridge for WASM containers.
 *
 * Ocre's native HTTP server (Zephyr's http_server subsystem) catches every
 * request, regardless of path or method, via a single fallback resource.
 * Each request is captured into a small pending-request table and handed to
 * the active reactive module instance's exported "onRequest(requestId)"
 * function (see ocre_dispatch.h) -- all routing logic lives in the
 * container's own AssemblyScript/C code, not in native code.
 *
 * The handler reads the request with the accessors below and answers with
 * ocre_http_respond_wasm(). If it never responds, the native side times the
 * request out and returns a 500.
 */

/** HTTP method values match Zephyr's enum http_method (zephyr/net/http/method.h). */
int ocre_http_get_method_wasm(wasm_exec_env_t exec_env, int32_t request_id);

/** Copies the request path (no query string) into buf, NUL-terminated.
 * Returns the path length (excluding NUL), or -1 if request_id is invalid or
 * buf is too small. */
int ocre_http_get_path_wasm(wasm_exec_env_t exec_env, int32_t request_id, char *buf, uint32_t buf_len);

/** Copies the raw query string (no leading '?'; empty string if none) into
 * buf, NUL-terminated. Returns the length (excluding NUL), or -1 on error. */
int ocre_http_get_query_wasm(wasm_exec_env_t exec_env, int32_t request_id, char *buf, uint32_t buf_len);

/** Copies the named request header's value into buf, NUL-terminated.
 * Returns the value length, 0 if the header wasn't found/captured, or -1 on
 * error. Header capture requires CONFIG_HTTP_SERVER_CAPTURE_HEADERS on the
 * device; without it this always returns 0. */
int ocre_http_get_header_wasm(wasm_exec_env_t exec_env, int32_t request_id, const char *name, char *buf,
			      uint32_t buf_len);

/** Copies the request body into buf, NUL-terminated (bodies are treated as
 * text; embedded NULs truncate). Returns the body length (excluding NUL), or
 * -1 on error. */
int ocre_http_get_body_wasm(wasm_exec_env_t exec_env, int32_t request_id, char *buf, uint32_t buf_len);

/** Answers a request captured by onRequest(). content_type/body may be NULL
 * for an empty response. Returns 0 on success, negative on error (e.g.
 * unknown/already-answered request_id). */
int ocre_http_respond_wasm(wasm_exec_env_t exec_env, int32_t request_id, int32_t status, const char *content_type,
			   const char *body);

#endif /* OCRE_HTTP_H */
