/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"
#include <zephyr/kernel.h>

static void zephyr_timer_cb(struct k_timer *ktimer)
{
	core_timer_t *otimer = CONTAINER_OF(ktimer, core_timer_t, timer);
	if (otimer->cb) {
		otimer->cb(otimer->user_data);
	}
}

int core_timer_init(core_timer_t *timer, core_timer_callback_t cb, void *user_data)
{
	if (!timer)
		return -1;
	timer->cb = cb;
	timer->user_data = user_data;
	k_timer_init(&timer->timer, zephyr_timer_cb, NULL);
	return 0;
}

int core_timer_start(core_timer_t *timer, int timeout_ms, int period_ms)
{
	if (!timer)
		return -1;
	k_timer_start(&timer->timer, K_MSEC(timeout_ms), K_MSEC(period_ms));
	return 0;
}

int core_timer_stop(core_timer_t *timer)
{
	if (!timer)
		return -1;
	k_timer_stop(&timer->timer);
	return 0;
}
