#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUF_SIZE 64

static int pid_fd = -1;

void ocre_pid_file_release(void)
{
	/* TODO: should we release the flock? */

	if (pid_fd >= 0) {
		close(pid_fd);
		pid_fd = -1;
	}
}

int ocre_pid_file_manage(const char *pid_file)
{
	int ret = -1;

	pid_fd = open(pid_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (pid_fd < 0) {
		fprintf(stderr, "Failed to open PID file '%s': %s\n", pid_file, strerror(errno));
		return -1;
	}

	/* TODO: should we really set CLOEXEC? */

	int flags = fcntl(pid_fd, F_GETFD);
	if (flags < 0) {
		fprintf(stderr, "Failed to get flags for PID file '%s': %s\n", pid_file, strerror(errno));
		goto finish;
	}

	flags |= FD_CLOEXEC;

	if (fcntl(pid_fd, F_SETFD, flags) < 0) {
		fprintf(stderr, "Failed to set flags for PID file '%s': %s\n", pid_file, strerror(errno));
		goto finish;
	}

	/* TODO: maybe check if the process is running? */

	/* TODO: do we need the flock? */

	struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0,
		.l_len = 0,
	};

	if (fcntl(pid_fd, F_WRLCK, &fl) < 0) {
		fprintf(stderr, "Failed to lock PID file '%s': %s\n", pid_file, strerror(errno));
		goto finish;
	}

	if (ftruncate(pid_fd, 0) < 0) {
		fprintf(stderr, "Failed to truncate PID file '%s': %s\n", pid_file, strerror(errno));
		goto finish;
	}

	char buf[BUF_SIZE];

	snprintf(buf, BUF_SIZE, "%ld\n", (long)getpid());
	if (write(pid_fd, buf, strlen(buf)) != (ssize_t)strlen(buf)) {
		fprintf(stderr, "Failed to write PID file '%s': %s\n", pid_file, strerror(errno));
		goto finish;
	}

	ret = 0;

finish:
	ocre_pid_file_release();
	return 0;
}
