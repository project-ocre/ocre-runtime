/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core_external.h"

#include <pthread.h>
#include <stdlib.h>

int core_mutex_init(core_mutex_t *mutex)
{
	if (!mutex)
		return -1;
	return pthread_mutex_init(&mutex->native_mutex, NULL);
}

int core_mutex_destroy(core_mutex_t *mutex)
{
	return pthread_mutex_destroy(&mutex->native_mutex);
}

int core_mutex_lock(core_mutex_t *mutex)
{
	return pthread_mutex_lock(&mutex->native_mutex);
}

int core_mutex_unlock(core_mutex_t *mutex)
{
	return pthread_mutex_unlock(&mutex->native_mutex);
}
