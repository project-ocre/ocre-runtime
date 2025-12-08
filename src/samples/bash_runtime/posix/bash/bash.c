#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <spawn.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <ocre/runtime/vtable.h>

#include <ocre/platform/log.h>

LOG_MODULE_REGISTER(bash_runtime);

struct bash_context {
	pid_t pid;
	char **argv;
	char **envp;
};

static int instance_execute(void *runtime_context)
{
	struct bash_context *context = runtime_context;

	// TODO: configure fileactions and attr

	int ret;
	ret = posix_spawnp(&context->pid, "/bin/bash", NULL, NULL, context->argv, context->envp);

	if (ret) {
		LOG_ERR("Failed to spawn process ret=%d errno=%d", ret, errno);
		return -1;
	}

	LOG_INF("Process spawned successfully PID=%d", context->pid);

	int status;

	pid_t pid = waitpid(context->pid, &status, 0);
	if (pid == -1) {
		LOG_ERR("Failed to wait for process");
		return -1;
	}

	LOG_INF("Waitpid returned %d status: %d", pid, WEXITSTATUS(status));

	LOG_INF("Container %p completed successfully", context);

	return WEXITSTATUS(status);
}

static int instance_thread_execute(void *arg)
{
	struct bash_context *context = arg;

	int ret = instance_execute(context);

	return ret;
}

static void *instance_create(const char *path, size_t stack_size, size_t heap_size, const char **capabilities,
			     const char **argv, const char **envp, const char **mounts)
{
	if (!path) {
		LOG_ERR("Invalid arguments");
		return NULL;
	}

	struct bash_context *context = malloc(sizeof(struct bash_context));
	if (!context) {
		LOG_ERR("Failed to allocate memory for BASH container size=%zu errno=%d", sizeof(struct bash_context),
			errno);
		return NULL;
	}

	memset(context, 0, sizeof(struct bash_context));

	for (const char **cap = capabilities; cap && *cap; cap++) {
		LOG_WRN("Capability '%s' not supported by runtime", *cap);
	}

	// For envp we can just keep a reference
	// as the container is guaranteed to only free it after our lifetime
	context->envp = (char **)envp;

	// We need to insert argv[0] and argv[1]. We can keep a shallow copy, because
	// the container is guaranteed to only free it after our lifetime
	int argc = 0;
	while (argv && argv[argc]) {
		argc++;
	}

	// 3 more: bash, script name and NULL
	context->argv = malloc(sizeof(char *) * (argc + 3));
	if (!context->argv) {
		LOG_ERR("Failed to allocate memory for argv");
		goto error_context;
	}

	memset(context->argv, 0, sizeof(char *) * (argc + 3));

	context->argv[0] = strdup("/bin/bash");
	if (!context->argv[0]) {
		goto error_argv;
	}

	context->argv[1] = strdup(path);
	if (!context->argv[1]) {
		goto error_argv;
	}

	int i;
	for (i = 0; i < argc; i++) {
		context->argv[i + 2] = strdup(argv[i]);
		if (!context->argv[i + 2]) {
			goto error_argv;
		}
	}

	context->argv[i + 2] = NULL;

	return context;

	// TODO: set up chroot, namespace, jails, whatever
	// configure fileactions here instead?

error_argv:
	free(context->argv[0]);
	free(context->argv[1]);
	free(context->argv);

error_context:
	free(context);

	return NULL;
}

static int instance_kill(void *runtime_context)
{
	struct bash_context *context = runtime_context;

	if (!context) {
		return -1;
	}

	int rc = kill(context->pid, SIGKILL);
	if (rc < 0) {
		LOG_ERR("Failed to kill process: rc=%d, errno=%d", rc, errno);
		return -1;
	}

	return 0;
}

static int instance_destroy(void *runtime_context)
{
	struct bash_context *context = runtime_context;

	if (!context) {
		return -1;
	}

	free(context->argv[0]);
	free(context->argv[1]);
	free(context->argv);

	free(context);

	return 0;
}

const struct ocre_runtime_vtable bash_vtable = {
	.runtime_name = "bash",
	.create = instance_create,
	.destroy = instance_destroy,
	.thread_execute = instance_thread_execute,
	.kill = instance_kill,
};
