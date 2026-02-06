/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _POSIX_C_SOURCE 200809L

#include "core_external.h"

#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>

uint32_t core_uptime_get(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

core_spinlock_key_t core_spinlock_lock(core_spinlock_t *lock)
{
	pthread_mutex_lock(&lock->mutex);
	return 0;
}

void core_spinlock_unlock(core_spinlock_t *lock, core_spinlock_key_t key)
{
	(void)key;
	pthread_mutex_unlock(&lock->mutex);
}
