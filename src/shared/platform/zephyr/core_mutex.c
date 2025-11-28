/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"
#include <zephyr/kernel.h>

int core_mutex_init(core_mutex_t *mutex)
{
	if (!mutex)
		return -1;
	k_mutex_init(&mutex->native_mutex);
	return 0;
}

int core_mutex_destroy(core_mutex_t *mutex)
{
	return 0;
}

int core_mutex_lock(core_mutex_t *mutex)
{
	return k_mutex_lock(&mutex->native_mutex, K_FOREVER);
}

int core_mutex_unlock(core_mutex_t *mutex)
{
	return k_mutex_unlock(&mutex->native_mutex);
}
