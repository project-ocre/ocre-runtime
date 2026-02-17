/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zcbor_common.h>

#include <ocre/ocre.h>

#include "../zcbor_helpers.h"
#include "waiters.h"
#include "handlers.h"

#define DEFAULT_PID_FILE   "ocre.pid"

#define SOCK_PATH	   "/tmp/ocre.sock"
#define RX_BUFFER_SIZE	   2048
#define TX_BUFFER_SIZE	   2048
#define STRING_BUFFER_SIZE 256
#define MAX_CONTAINERS	   64
#define MAX_STRING_ARRAY   32

/* Global ocre context for the server */
static struct ocre_context *ctx = NULL;

static int s = -1;
static bool should_exit = false;

static void terminate(int signum)
{
	fprintf(stderr, "Received signal %d\n", signum);
	should_exit = true;
	if (s != -1) {
		fprintf(stderr, "Closing socket\n");
		close(s);
	}
}

static int usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [options]\n", argv0);
	fprintf(stderr, "\nStart the Ocre daemon.\n");
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "  -v                       Enable verbose logging\n");
	fprintf(stderr, "  -w WORKDIR               Specifies a working directory\n");
	fprintf(stderr, "  -H LISTEN_ADDR           Specifies a socket to listen on\n");
	fprintf(stderr, "  -P PID_FILE              Use a PID file (default: " DEFAULT_PID_FILE ")\n");
	fprintf(stderr, "  -h                       Display this help message\n");
	fprintf(stderr, "\nOption '-H' can be supplied multiple times and take the format:\n");
	fprintf(stderr, "  fd://SOCKET_PATH         UNIX domain socket path\n");
	fprintf(stderr, "  tcp://HOST:PORT          TCP socket path\n");
	fprintf(stderr, "  fd://                    Socket activation mode\n");
	return -1;
}

int main(int argc, char *argv[])
{
	int opt;
	char *workdir = NULL;
	char *pid_file = NULL;
	char *ctrl_path = NULL;
	bool verbose = false;

	while ((opt = getopt(argc, argv, "+hw:vH:P:")) != -1) {
		switch (opt) {
			case 'v': {
				if (verbose) {
					fprintf(stderr, "'-v' can be set only once\n\n");
					return usage(argv[0]);
					return -1;
				}

				verbose = true;
				continue;
			}
			case 'w': {
				if (workdir) {
					fprintf(stderr, "Workdir can be set only once\n\n");
					usage(argv[0]);
					return -1;
				}

				if (optarg) {
					fprintf(stderr, "Invalid workdir\n\n");
					usage(argv[0]);
					return -1;
				}

				workdir = optarg;
				continue;
			}
			case 'H': {
				if (ctrl_path) {
					fprintf(stderr, "Control socket can be set only once\n\n");
					usage(argv[0]);
					return -1;
				}

				if (optarg) {
					fprintf(stderr, "Invalid control socket\n\n");
					return -1;
				}

				ctrl_path = optarg;
				continue;
			}
			case 'P': {
				if (pid_file) {
					fprintf(stderr, "PID file can be set only once\n\n");
					usage(argv[0]);
					return -1;
				}

				if (optarg) {
					fprintf(stderr, "Invalid PID file\n\n");
					usage(argv[0]);
					return -1;
				}

				pid_file = optarg;
				continue;
			}
			case 'h': {
				usage(argv[0]);
				return 0;
			}
			case '?': {
				fprintf(stderr, "Invalid option: '%c'\n", optopt);
				usage(argv[0]);
				return -1;
			}
		}
	}

	if (!pid_file) {
		fprintf(stderr, "Using default PID file '%s'\n", DEFAULT_PID_FILE);
		pid_file = DEFAULT_PID_FILE;
	}

	if (!ctrl_path) {
		fprintf(stderr, "Using default control socket path '%s'\n", SOCK_PATH);
		ctrl_path = SOCK_PATH;
	}

	// if (!workdir) {
	// 	fprintf(stderr, "Using default working directory\n");
	// }

	// /* Read current PID file */
	// int pidfile = open(pid_file, O_RDONLY);
	// if (pidfile >= 0) {
	// 	fprintf(stderr, "PID file '%s' already exists. Will check it\n", pid_file);
	// 	char pids[16];
	// 	if (read(pidfile, pids, sizeof(pids)) != sizeof(pids)) {
	// 		fprintf(stderr, "Failed to read PID file '%s'\n", pid_file);
	// 		close(pidfile);
	// 		return -1;
	// 	}

	// 	int pid = atoi(pids);

	// 	if (pid >= 0) {
	// 		fprintf(stderr, "Checking PID %d\n", pid);
	// 		if (waitpid(pid, NULL, WNOHANG) == pid) {
	// 			fprintf(stderr, "Process %d is running\n", pid);
	// 			fprintf(stderr, "Process %d is not running\n", pid);
	// 		} else {
	// 		}
	// 	}
	// }

	// close(pidfile);

	// int rc = cleanup_socket(SOCK_PATH);
	// if (rc < 0) {
	// 	fprintf(stderr, "Failed to cleanup socket file\n");
	// 	return 1;
	// }

	int rc = ocre_initialize(NULL);
	if (rc < 0) {
		fprintf(stderr, "Failed to initialize runtimes\n");
		return 1;
	}
	ctx = ocre_create_context(workdir);
	if (!ctx) {
		fprintf(stderr, "Failed to create ocre context\n");
		return 1;
	}
	struct sockaddr_un remote, local = {
					   .sun_family = AF_UNIX,
				   };
	int s2;
	uint8_t rx_buf[RX_BUFFER_SIZE];
	uint8_t tx_buf[TX_BUFFER_SIZE];

	if ((s = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) == -1) {
		perror("socket");
		return 1;
	}

	if (strlen(ctrl_path) > sizeof(local.sun_path) - 1) {
		fprintf(stderr, "Control path too long\n");
		return 1;
	}

	strcpy(local.sun_path, ctrl_path);
	size_t len = strlen(local.sun_path) + sizeof(local.sun_family);

	unlink(local.sun_path);
	if (bind(s, (struct sockaddr *)&local, len) == -1) {
		fprintf(stderr, "Failed to bind socket '%s'\n", local.sun_path);
		perror("bind");
		exit(1);
	}

	/* Install signal handler */

	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = terminate;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	if (listen(s, 5) == -1) {
		perror("listen");
		exit(1);
	}

	printf("Ocre daemon started, listening on %s\n", SOCK_PATH);

	for (;;) {

		if (should_exit) {
			close(s);
			exit(1);
		}

		int n;
		printf("Waiting for a connection...\n");
		socklen_t slen = sizeof(remote);
		if ((s2 = accept(s, (struct sockaddr *)&remote, &slen)) == -1) {
			perror("accept");
			continue;
		}

		printf("Connected.\n");

		// do {
		n = recv(s2, rx_buf, sizeof(rx_buf), 0);
		if (n <= 0) {
			if (n < 0)
				perror("recv");
			else
				printf("Client disconnected\n");
			continue;
		}

		/* Process the request and generate response */
		int response_len = process_request(ctx, s2, rx_buf, n, tx_buf, sizeof(tx_buf));

		if (response_len == 0) {
			continue;
		}

		if (response_len < 0) {
			printf("Failed to process request\n");
			continue;
		}

		if (response_len > 0) {
			if (send(s2, tx_buf, response_len, 0) < 0) {
				perror("send");
				break;
			}

			printf("Response sent (%d bytes)\n", response_len);
		}
		// } while (1);

		close(s2);
	}

	return 0;
}
