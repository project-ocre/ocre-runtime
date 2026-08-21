/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_http.h"
#include "../ocre_dispatch/ocre_dispatch.h"

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <pthread.h>

#include <zephyr/init.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ocre_http, CONFIG_OCRE_LOG_LEVEL);

#ifndef CONFIG_OCRE_HTTP_SERVER_PORT
#define CONFIG_OCRE_HTTP_SERVER_PORT 8081
#endif

#define OCRE_HTTP_MAX_PENDING_REQUESTS 4
#define OCRE_HTTP_MAX_CAPTURED_HEADERS 8
#define OCRE_HTTP_HEADER_NAME_SIZE     64
#define OCRE_HTTP_HEADER_VALUE_SIZE    256
#define OCRE_HTTP_MAX_PATH_LEN	       128
#define OCRE_HTTP_MAX_QUERY_LEN	       128
#define OCRE_HTTP_MAX_CONTENT_TYPE_LEN 64

static uint16_t ocre_http_port = CONFIG_OCRE_HTTP_SERVER_PORT;

/* ============================================================
 * Per-connection request capture: accumulates a request's path, query,
 * headers and (for POST/PUT/PATCH) body across the one-or-more callback
 * invocations Zephyr's http_server makes while a request streams in, then
 * hands the finished request to ocre_dispatch_call_on_request().
 * ============================================================ */

struct captured_header {
	char name[OCRE_HTTP_HEADER_NAME_SIZE];
	char value[OCRE_HTTP_HEADER_VALUE_SIZE];
};

struct client_capture {
	struct http_client_ctx *client; /* NULL = free slot */
	bool capturing;			/* path/method/headers captured for the in-flight request */
	enum http_method method;
	char path[OCRE_HTTP_MAX_PATH_LEN];
	char query[OCRE_HTTP_MAX_QUERY_LEN];
	char *body;
	size_t body_len;
	size_t body_cap;
	struct captured_header headers[OCRE_HTTP_MAX_CAPTURED_HEADERS];
	size_t header_count;
};

static pthread_mutex_t captures_lock = PTHREAD_MUTEX_INITIALIZER;
static struct client_capture captures[OCRE_HTTP_MAX_PENDING_REQUESTS];

/* Caller must hold captures_lock. */
static struct client_capture *get_capture_for_client(struct http_client_ctx *client)
{
	struct client_capture *free_slot = NULL;

	for (int i = 0; i < OCRE_HTTP_MAX_PENDING_REQUESTS; i++) {
		if (captures[i].client == client) {
			return &captures[i];
		}
		if (!free_slot && captures[i].client == NULL) {
			free_slot = &captures[i];
		}
	}

	if (free_slot) {
		free_slot->client = client;
	}

	return free_slot;
}

static void reset_capture(struct client_capture *cap)
{
	free(cap->body);
	struct http_client_ctx *client = cap->client;

	memset(cap, 0, sizeof(*cap));
	cap->client = client;
}

static void release_capture_for_client(struct http_client_ctx *client)
{
	pthread_mutex_lock(&captures_lock);

	for (int i = 0; i < OCRE_HTTP_MAX_PENDING_REQUESTS; i++) {
		if (captures[i].client == client) {
			free(captures[i].body);
			memset(&captures[i], 0, sizeof(captures[i]));
			break;
		}
	}

	pthread_mutex_unlock(&captures_lock);
}

static bool method_has_body(enum http_method method)
{
	return method == HTTP_POST || method == HTTP_PUT || method == HTTP_PATCH;
}

static void capture_append_body(struct client_capture *cap, const uint8_t *data, size_t len)
{
	if (!data || len == 0) {
		return;
	}

	size_t needed = cap->body_len + len + 1;

	if (needed > cap->body_cap) {
		size_t new_cap = cap->body_cap ? cap->body_cap * 2 : 256;

		while (new_cap < needed) {
			new_cap *= 2;
		}

		char *new_body = realloc(cap->body, new_cap);

		if (!new_body) {
			LOG_WRN("Failed to grow request body buffer to %zu bytes", new_cap);
			return;
		}

		cap->body = new_body;
		cap->body_cap = new_cap;
	}

	memcpy(cap->body + cap->body_len, data, len);
	cap->body_len += len;
	cap->body[cap->body_len] = '\0';
}

/* ============================================================
 * Pending request table: the finished, captured requests currently being
 * handed to (or answered by) the active container's onRequest() export.
 * ============================================================ */

struct http_pending_request {
	bool in_use;
	int32_t id;

	enum http_method method;
	char path[OCRE_HTTP_MAX_PATH_LEN];
	char query[OCRE_HTTP_MAX_QUERY_LEN];
	char *body;
	size_t body_len;

	struct captured_header headers[OCRE_HTTP_MAX_CAPTURED_HEADERS];
	size_t header_count;

	bool responded;
	int status;
	char content_type[OCRE_HTTP_MAX_CONTENT_TYPE_LEN];
	char *resp_body;
	size_t resp_body_len;
};

static pthread_mutex_t requests_lock = PTHREAD_MUTEX_INITIALIZER;
static struct http_pending_request requests[OCRE_HTTP_MAX_PENDING_REQUESTS];
static int32_t next_request_id = 1;

static struct http_pending_request *alloc_request(void)
{
	pthread_mutex_lock(&requests_lock);

	for (int i = 0; i < OCRE_HTTP_MAX_PENDING_REQUESTS; i++) {
		if (!requests[i].in_use) {
			memset(&requests[i], 0, sizeof(requests[i]));
			requests[i].in_use = true;
			requests[i].id = next_request_id++;
			pthread_mutex_unlock(&requests_lock);
			return &requests[i];
		}
	}

	pthread_mutex_unlock(&requests_lock);
	return NULL;
}

static void free_request(struct http_pending_request *req)
{
	pthread_mutex_lock(&requests_lock);
	free(req->body);
	free(req->resp_body);
	memset(req, 0, sizeof(*req));
	pthread_mutex_unlock(&requests_lock);
}

/* Returns the slot matching request_id with requests_lock held, or NULL
 * (lock released) if not found. Caller must unlock via requests_lock when
 * done, only in the found case. */
static struct http_pending_request *lock_request(int32_t request_id)
{
	pthread_mutex_lock(&requests_lock);

	for (int i = 0; i < OCRE_HTTP_MAX_PENDING_REQUESTS; i++) {
		if (requests[i].in_use && requests[i].id == request_id) {
			return &requests[i];
		}
	}

	pthread_mutex_unlock(&requests_lock);
	return NULL;
}

/* ============================================================
 * Native API: accessors + respond(), called by the active container's
 * onRequest() handler.
 * ============================================================ */

static int copy_out(const char *src, size_t src_len, char *buf, uint32_t buf_len)
{
	if (!buf || buf_len == 0 || src_len + 1 > buf_len) {
		return -1;
	}

	memcpy(buf, src, src_len);
	buf[src_len] = '\0';
	return (int)src_len;
}

int ocre_http_get_method_wasm(wasm_exec_env_t exec_env, int32_t request_id)
{
	ARG_UNUSED(exec_env);

	struct http_pending_request *req = lock_request(request_id);

	if (!req) {
		return -1;
	}

	int method = (int)req->method;

	pthread_mutex_unlock(&requests_lock);
	return method;
}

int ocre_http_get_path_wasm(wasm_exec_env_t exec_env, int32_t request_id, char *buf, uint32_t buf_len)
{
	ARG_UNUSED(exec_env);

	struct http_pending_request *req = lock_request(request_id);

	if (!req) {
		return -1;
	}

	int ret = copy_out(req->path, strlen(req->path), buf, buf_len);

	pthread_mutex_unlock(&requests_lock);
	return ret;
}

int ocre_http_get_query_wasm(wasm_exec_env_t exec_env, int32_t request_id, char *buf, uint32_t buf_len)
{
	ARG_UNUSED(exec_env);

	struct http_pending_request *req = lock_request(request_id);

	if (!req) {
		return -1;
	}

	int ret = copy_out(req->query, strlen(req->query), buf, buf_len);

	pthread_mutex_unlock(&requests_lock);
	return ret;
}

int ocre_http_get_header_wasm(wasm_exec_env_t exec_env, int32_t request_id, const char *name, char *buf,
			      uint32_t buf_len)
{
	ARG_UNUSED(exec_env);

	if (!name) {
		return -1;
	}

	struct http_pending_request *req = lock_request(request_id);

	if (!req) {
		return -1;
	}

	int ret = 0;

	for (size_t i = 0; i < req->header_count; i++) {
		if (strcasecmp(req->headers[i].name, name) == 0) {
			ret = copy_out(req->headers[i].value, strlen(req->headers[i].value), buf, buf_len);
			break;
		}
	}

	pthread_mutex_unlock(&requests_lock);
	return ret;
}

int ocre_http_get_body_wasm(wasm_exec_env_t exec_env, int32_t request_id, char *buf, uint32_t buf_len)
{
	ARG_UNUSED(exec_env);

	struct http_pending_request *req = lock_request(request_id);

	if (!req) {
		return -1;
	}

	int ret = copy_out(req->body ? req->body : "", req->body_len, buf, buf_len);

	pthread_mutex_unlock(&requests_lock);
	return ret;
}

int ocre_http_respond_wasm(wasm_exec_env_t exec_env, int32_t request_id, int32_t status, const char *content_type,
			   const char *body)
{
	ARG_UNUSED(exec_env);

	struct http_pending_request *req = lock_request(request_id);

	if (!req) {
		return -1;
	}

	if (req->responded) {
		pthread_mutex_unlock(&requests_lock);
		return -1;
	}

	req->status = status;

	if (content_type) {
		strncpy(req->content_type, content_type, sizeof(req->content_type) - 1);
	}

	if (body) {
		size_t len = strlen(body);

		req->resp_body = malloc(len + 1);
		if (req->resp_body) {
			memcpy(req->resp_body, body, len + 1);
			req->resp_body_len = len;
		}
	}

	req->responded = true;

	pthread_mutex_unlock(&requests_lock);
	return 0;
}

/* ============================================================
 * The single native fallback HTTP resource: catches every request
 * regardless of path/method and dispatches it to the active container.
 * ============================================================ */

static int ocre_http_fallback_handler(struct http_client_ctx *client, enum http_transaction_status status,
				      const struct http_request_ctx *request_ctx,
				      struct http_response_ctx *response_ctx, void *user_data)
{
	ARG_UNUSED(user_data);

	if (status == HTTP_SERVER_TRANSACTION_COMPLETE || status == HTTP_SERVER_TRANSACTION_ABORTED) {
		release_capture_for_client(client);
		return 0;
	}

	pthread_mutex_lock(&captures_lock);

	struct client_capture *cap = get_capture_for_client(client);

	if (!cap) {
		pthread_mutex_unlock(&captures_lock);
		response_ctx->status = HTTP_500_INTERNAL_SERVER_ERROR;
		response_ctx->final_chunk = true;
		return 0;
	}

	if (!cap->capturing) {
		cap->capturing = true;
		cap->method = client->method;

		const char *url = (const char *)client->url_buffer;
		const char *qmark = strchr(url, '?');
		size_t path_len = qmark ? (size_t)(qmark - url) : strlen(url);

		if (path_len >= sizeof(cap->path)) {
			path_len = sizeof(cap->path) - 1;
		}
		memcpy(cap->path, url, path_len);
		cap->path[path_len] = '\0';

		if (qmark) {
			strncpy(cap->query, qmark + 1, sizeof(cap->query) - 1);
		}

		if (request_ctx->headers_status != HTTP_HEADER_STATUS_NONE) {
			for (size_t i = 0; i < request_ctx->header_count && i < OCRE_HTTP_MAX_CAPTURED_HEADERS; i++) {
				strncpy(cap->headers[i].name, request_ctx->headers[i].name,
					sizeof(cap->headers[i].name) - 1);
				strncpy(cap->headers[i].value, request_ctx->headers[i].value,
					sizeof(cap->headers[i].value) - 1);
				cap->header_count++;
			}
		}
	}

	if (method_has_body(cap->method)) {
		capture_append_body(cap, request_ctx->data, request_ctx->data_len);
	}

	if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
		pthread_mutex_unlock(&captures_lock);
		return 0;
	}

	/* Request fully captured -- move it into a pending-request slot and
	 * dispatch it to the active container. */

	struct http_pending_request *preq = alloc_request();

	if (!preq) {
		LOG_WRN("HTTP request table full; rejecting request");
		reset_capture(cap);
		pthread_mutex_unlock(&captures_lock);
		response_ctx->status = HTTP_503_SERVICE_UNAVAILABLE;
		response_ctx->final_chunk = true;
		return 0;
	}

	preq->method = cap->method;
	strncpy(preq->path, cap->path, sizeof(preq->path) - 1);
	strncpy(preq->query, cap->query, sizeof(preq->query) - 1);
	memcpy(preq->headers, cap->headers, sizeof(preq->headers));
	preq->header_count = cap->header_count;
	preq->body = cap->body;
	preq->body_len = cap->body_len;
	cap->body = NULL; /* ownership moved to preq */

	int32_t request_id = preq->id;

	reset_capture(cap);
	pthread_mutex_unlock(&captures_lock);

	bool called = ocre_dispatch_call_on_request(request_id);

	pthread_mutex_lock(&requests_lock);

	/* preq may have been freed by nothing else (only we free it), so it's
	 * still valid; re-check responded/status/body directly. */
	if (!called || !preq->responded) {
		response_ctx->status = HTTP_404_NOT_FOUND;
		response_ctx->body = NULL;
		response_ctx->body_len = 0;
	} else {
		static __thread char content_type_buf[OCRE_HTTP_MAX_CONTENT_TYPE_LEN];
		static __thread struct http_header content_type_header;

		response_ctx->status = (enum http_status)preq->status;
		response_ctx->body = (const uint8_t *)preq->resp_body;
		response_ctx->body_len = preq->resp_body_len;

		if (preq->content_type[0]) {
			strncpy(content_type_buf, preq->content_type, sizeof(content_type_buf) - 1);
			content_type_header.name = "Content-Type";
			content_type_header.value = content_type_buf;
			response_ctx->headers = &content_type_header;
			response_ctx->header_count = 1;
		}
	}

	response_ctx->final_chunk = true;

	pthread_mutex_unlock(&requests_lock);

	free_request(preq);

	return 0;
}

static struct http_resource_detail_dynamic ocre_http_fallback_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST) | BIT(HTTP_PUT) |
							     BIT(HTTP_DELETE) | BIT(HTTP_HEAD) | BIT(HTTP_OPTIONS) |
							     BIT(HTTP_PATCH),
			.content_type = "text/plain",
		},
	.cb = ocre_http_fallback_handler,
	.user_data = NULL,
};

/* concurrent=4 matches CONFIG_HTTP_SERVER_MAX_CLIENTS (raised to 4 in the
 * sample's prj.conf); backlog=8 gives room for several pending/half-closed
 * connections so one stuck client can't wedge the server. The fallback
 * resource (last arg before service-level config) catches every path --
 * there are no other registered resources, so routing is entirely up to
 * the container's onRequest() handler. */
HTTP_SERVICE_DEFINE(ocre_http_service, NULL, &ocre_http_port, 4, 8, NULL,
		    (struct http_resource_detail *)&ocre_http_fallback_detail, NULL);

static int ocre_http_server_init(void)
{
	int ret = http_server_start();

	if (ret < 0) {
		LOG_ERR("Failed to start Ocre HTTP server: %d", ret);
	}

	return ret;
}

SYS_INIT(ocre_http_server_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
