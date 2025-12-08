/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "core_external.h"

static void posix_timer_cb(union sigval sv)
{
	core_timer_t *timer = (core_timer_t *)sv.sival_ptr;
	if (timer->cb) {
		timer->cb(timer->user_data);
	}
}

int core_timer_init(core_timer_t *timer, core_timer_callback_t cb, void *user_data)
{
	if (!timer)
		return -1;
	struct sigevent sev = {0};
	timer->cb = cb;
	timer->user_data = user_data;
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = posix_timer_cb;
	sev.sigev_value.sival_ptr = timer;
	return timer_create(CLOCK_REALTIME, &sev, &timer->timerid);
}

int core_timer_start(core_timer_t *timer, int timeout_ms, int period_ms)
{
	if (!timer)
		return -1;
	struct itimerspec its;
	if (timeout_ms == 0 && period_ms > 0) {
		its.it_value.tv_sec = period_ms / 1000;
		its.it_value.tv_nsec = (period_ms % 1000) * 1000000;
	} else {
		its.it_value.tv_sec = timeout_ms / 1000;
		its.it_value.tv_nsec = (timeout_ms % 1000) * 1000000;
	}
	its.it_interval.tv_sec = period_ms / 1000;
	its.it_interval.tv_nsec = (period_ms % 1000) * 1000000;
	return timer_settime(timer->timerid, 0, &its, NULL);
}

int core_timer_stop(core_timer_t *timer)
{
	if (!timer)
		return -1;
	struct itimerspec its = {0};
	return timer_settime(timer->timerid, 0, &its, NULL);
}

int core_timer_delete(core_timer_t *timer)
{
	if (!timer)
		return -1;

	fprintf(stderr, "FINAL TIMER DELETEEEE!!!!!!!!\n");
	return timer_delete(timer->timerid);
}
