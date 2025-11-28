/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

static void *thread_entry(void *arg)
{
	struct {
		core_thread_func_t func;
		void *user_arg;
		char name[16];
	} *entry = arg;

	if (entry->name[0])
		pthread_setname_np(pthread_self(), entry->name);

	entry->func(entry->user_arg);
	free(entry);
	return NULL;
}

int core_thread_create(core_thread_t *thread, core_thread_func_t func, void *arg, const char *name, size_t stack_size,
		       int priority)
{
	if (!thread || !func)
		return -1;
	pthread_attr_t attr;
	int ret;

	if (stack_size < PTHREAD_STACK_MIN) {
		fprintf(stderr, "STACK_SIZE must be at least PTHREAD_STACK_MIN (%zu)\n", (size_t)PTHREAD_STACK_MIN);
		return -1;
	}

	thread->stack_size = stack_size;
	if (posix_memalign(&thread->stack, sysconf(_SC_PAGESIZE), stack_size) != 0) {
		perror("posix_memalign");
		return -1;
	}

	pthread_attr_init(&attr);
	pthread_attr_setstack(&attr, thread->stack, stack_size);

	// Prepare entry struct for name passing
	struct {
		core_thread_func_t func;
		void *user_arg;
		char name[16];
	} *entry = malloc(sizeof(*entry));
	entry->func = func;
	entry->user_arg = arg;
	strncpy(entry->name, name ? name : "", sizeof(entry->name) - 1);
	entry->name[sizeof(entry->name) - 1] = '\0';

	ret = pthread_create(&thread->tid, &attr, thread_entry, entry);
	pthread_attr_destroy(&attr);
	if (ret != 0) {
		free(thread->stack);
		free(entry);
		return -1;
	}
	return 0;
}

void core_thread_destroy(core_thread_t *thread)
{
	if (!thread)
		return;
	pthread_cancel(thread->tid);
	pthread_join(thread->tid, NULL);
	free(thread->stack);
}
