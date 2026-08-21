/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_http_client.h"

#include <string.h>
#include <unistd.h>

#include <zephyr/net/http/client.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/arpa/inet.h>

#ifndef CONFIG_OCRE_HTTP_CLIENT_TIMEOUT_MS
#define CONFIG_OCRE_HTTP_CLIENT_TIMEOUT_MS 5000
#endif

#define OCRE_HTTP_CLIENT_RECV_BUF_LEN 512

struct ocre_http_client_state {
	char *out_buf;
	uint32_t out_buf_len;
	size_t out_len;
	bool failed;
};

static int response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
	struct ocre_http_client_state *st = user_data;

	ARG_UNUSED(final_data);

	if (rsp->http_status_code != 0 && rsp->http_status_code != 200) {
		st->failed = true;
		return -1;
	}

	if (rsp->body_frag_len == 0) {
		return 0;
	}

	/* out_buf_len always leaves room for the NUL below (buf_len == 0 is
	 * rejected before this callback can run). */
	size_t space = st->out_buf_len - 1 - st->out_len;
	size_t copy_len = rsp->body_frag_len < space ? rsp->body_frag_len : space;

	if (copy_len > 0) {
		memcpy(st->out_buf + st->out_len, rsp->body_frag_start, copy_len);
		st->out_len += copy_len;
		st->out_buf[st->out_len] = '\0';
	}

	return 0;
}

int ocre_http_client_get_wasm(wasm_exec_env_t exec_env, const char *host, int32_t port, const char *path, char *buf,
			      uint32_t buf_len)
{
	ARG_UNUSED(exec_env);

	if (!host || !path || !buf || buf_len == 0 || port <= 0 || port > 65535) {
		return -1;
	}

	buf[0] = '\0';

	struct sockaddr_in addr;
	static uint8_t recv_buf[OCRE_HTTP_CLIENT_RECV_BUF_LEN];
	struct ocre_http_client_state st = {
		.out_buf = buf,
		.out_buf_len = buf_len,
	};
	struct http_request req;
	int sock;
	int ret;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);

	if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
		return -1;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(sock);
		return -1;
	}

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = path;
	req.host = host;
	req.protocol = "HTTP/1.1";
	req.response = response_cb;
	req.recv_buf = recv_buf;
	req.recv_buf_len = sizeof(recv_buf);

	ret = http_client_req(sock, &req, CONFIG_OCRE_HTTP_CLIENT_TIMEOUT_MS, &st);

	close(sock);

	if (ret < 0 || st.failed) {
		return -1;
	}

	return (int)st.out_len;
}
