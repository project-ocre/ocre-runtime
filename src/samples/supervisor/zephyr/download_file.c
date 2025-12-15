#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <zephyr/net/http/parser_url.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/http/status.h>

#define OCRE_DOWNLOAD_RESPONSE_BUFFER_SIZE (256)

static int response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
	int fd = *(int *)user_data;

	if (rsp->http_status_code != HTTP_200_OK) {
		fprintf(stderr, "Got invalid HTTP status code %d\n", rsp->http_status_code);
		return -1;
	}

	if (rsp->body_frag_start && rsp->body_frag_len) {
		size_t body_frag_len = rsp->data_len - (rsp->body_frag_start - rsp->recv_buf);

		int rc = write(fd, rsp->body_frag_start, body_frag_len);
		if (rc < 0) {
			fprintf(stderr, "Failed to write file: %d\n", rc);
			return -1;
		}
	}

	return 0;
}

int ocre_download_file(const char *url, const char *filepath)
{
	int rc = -1;
	char *hostname = NULL;
	struct addrinfo *res = NULL;
	char *response = NULL;

	/* Parse URL */

	struct http_parser_url parsed_url;
	http_parser_url_init(&parsed_url);
	http_parser_parse_url(url, strlen(url), 0, &parsed_url);

	if (!(parsed_url.field_set & (1 << UF_SCHEMA))) {
		fprintf(stderr, "Missing URL schema. Use 'http:// in URL\n");
		goto finish;
	}

	if (!(parsed_url.field_set & (1 << UF_PATH))) {
		fprintf(stderr, "Missing URL path. Use 'http://<url>/<path> in URL\n");
		goto finish;
	}

	uint16_t port = 0;
	char port_str[6];

	if (!strncmp("http", url + parsed_url.field_data[UF_SCHEMA].off, parsed_url.field_data[UF_SCHEMA].len)) {
		port = 80;
		strcpy(port_str, "80");
	// } else if (!strncmp("https", url + parsed_url.field_data[UF_SCHEMA].off,
	// 		    parsed_url.field_data[UF_SCHEMA].len)) {
	// 	port = 443;
	// 	strcpy(port_str, "443");
	} else {
		fprintf(stderr, "Unsupported URL schema: '%s'", url + parsed_url.field_data[UF_SCHEMA].off);
		goto finish;
	}

	if (parsed_url.field_set & (1 << UF_PORT)) {
		port = parsed_url.port;
		memcpy(port_str, url + parsed_url.field_data[UF_PORT].off, parsed_url.field_data[UF_PORT].len);
		port_str[parsed_url.field_data[UF_PORT].len] = '\0';
	}

	if (!(parsed_url.field_set & (1 << UF_HOST))) {
		fprintf(stderr, "Missing URL host. Use 'http://<url> in URL\n");
		goto finish;
	}

	response = malloc(OCRE_DOWNLOAD_RESPONSE_BUFFER_SIZE);
	if (!response) {
		fprintf(stderr, "Failed to allocate memory for response buffer\n");
		goto finish;
	}

	/* Resolve hostname */

	hostname = malloc(parsed_url.field_data[UF_HOST].len + 1);
	if (!hostname) {
		fprintf(stderr, "Failed to allocate memory for hostname");
		goto finish;
	}

	memcpy(hostname, url + parsed_url.field_data[UF_HOST].off, parsed_url.field_data[UF_HOST].len);
	hostname[parsed_url.field_data[UF_HOST].len] = '\0';

	static struct addrinfo hints;
	int st;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	st = getaddrinfo(hostname, port_str, &hints, &res);

	printf("getaddrinfo status: %d\n", st);

	if (st != 0) {
		fprintf(stderr, "Unable to resolve address: %s\n", hostname);
		goto finish;
	}

	/* Create file */

	int fd = open(filepath, O_CREAT | O_WRONLY, 0644);
	if (fd < 0) {
		fprintf(stderr, "Failed to create file '%s'\n", filepath);
		goto finish;
	}

	fprintf(stderr, "created file %s\n", filepath);

	/* Socket */

	int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (sock < 0) {
		fprintf(stderr, "Error %d on socket()\n", sock);
		goto finish;
	}

	/* Connection */

	st = connect(sock, res->ai_addr, res->ai_addrlen);
	if (st < 0) {
		fprintf(stderr, "Error %d on connect(), errno: %d\n", st, errno);
		goto finish;
	}

	/* HTTP Request */

	struct http_request req;

	int32_t timeout = 3 * MSEC_PER_SEC;

	memset(&req, 0, sizeof(req));

	req.method = HTTP_GET;
	req.url = url + parsed_url.field_data[UF_PATH].off;
	req.host = hostname;
	req.protocol = "HTTP/1.1";
	req.response = response_cb;
	req.recv_buf = response;
	req.recv_buf_len = OCRE_DOWNLOAD_RESPONSE_BUFFER_SIZE;

	st = http_client_req(sock, &req, timeout, (void *)&fd);
	fprintf(stderr, "st is %d\n", st);
	if (st < 0) {
		fprintf(stderr, "HTTP Client error %d\n", st);
		goto finish;
	} else {
		fprintf(stderr, "finished downloading file!!!!\n");
	}

	rc = 0;

finish:

	if (fd >= 0) {
		close(fd);
	}

	if (sock >= 0) {
		close(sock);
	}

	if (rc) {
		unlink(filepath);
	}

	if (res) {
		freeaddrinfo(res);
	}

	free(hostname);

	free(response);

	return rc;
}
