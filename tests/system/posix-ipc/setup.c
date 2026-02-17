#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <spawn.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

extern void test_setUp(void);
extern void test_tearDown(void);

static pid_t ocred_pid;

void setUp(void)
{

#if 1
	if (chdir("ocre")) {
		perror("chdir");
	}

	if (unlink("/tmp/ocre.sock") == -1) {
		perror("setUp unlink");
	}

	char *argv[] = {"./src/samples/supervisor/posix/server/ocred", NULL};
	char *envp[] = {NULL};
	printf("will spawn ocred\n");
	int rc = posix_spawnp(&ocred_pid, "./src/samples/supervisor/posix/server/ocred", NULL, NULL, argv, envp);
	printf("spawned ocred PID %d\n", rc);

	// try to connect to socket

	int s;
	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	while (1) {
		struct sockaddr_un remote = {
			.sun_family = AF_UNIX,
		};

		strcpy(remote.sun_path, "/tmp/ocre.sock");
		size_t len = strlen(remote.sun_path) + sizeof(remote.sun_family);
		fprintf(stderr, "connecting to %s\n", remote.sun_path);
		if (connect(s, (struct sockaddr *)&remote, len) == -1) {
			perror("connect");
			usleep(100000);
			continue;
		}

		fprintf(stderr, "connected to %s\n", remote.sun_path);
		break;
	}

	close(s);

	if (chdir("..")) {
		perror("chdir");
	}

	// sleep(1);

	test_setUp();
}

void tearDown(void)
{
	test_tearDown();
	printf("sending SIGTERM to ocred\n");
	kill(ocred_pid, SIGTERM);
	waitpid(ocred_pid, NULL, 0);
	if (unlink("/tmp/ocre.sock") == -1) {
		perror("tearDown unlink");
	}
}
