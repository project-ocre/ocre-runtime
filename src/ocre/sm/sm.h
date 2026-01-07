/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SM_H
#define SM_H

#include "ocre_core_external.h"

#define MAX_TIMERS 3

#define SM_RETURN_IF_EVENT_HANDLED(o)                                                                                  \
	if (((struct sm_ctx *)o)->event.handled)                                                                       \
	return SMF_EVENT_HANDLED
#define SM_MARK_EVENT_HANDLED(o) ((struct sm_ctx *)o)->event.handled = true
#define SM_GET_EVENT(o)		 ((struct sm_ctx *)o)->event.msg
#define SM_GET_CUSTOM_CTX(o)	 ((struct sm_ctx *)o)->custom_ctx

#define EVENT_LOG_MSG(fmt, event) LOG_DBG(fmt, event)

struct sm_ctx {
	struct smf_ctx ctx;
	struct {
		bool handled;
		void *msg;
	} event;
	void *custom_ctx;
};

typedef struct state_machine {
	core_mq_t *msgq;
	core_timer_t timers[MAX_TIMERS];
	struct sm_ctx ctx;	     /*!< State machine context */
	const struct smf_state *hsm; /*!< State machine states */
} state_machine_t;

int sm_transition(state_machine_t *sm, int target_state);

int sm_run(state_machine_t *sm, int initial_state);

int sm_init_event_timer(state_machine_t *sm, int timer_id, void *timer_cb);

int sm_set_event_timer(state_machine_t *sm, int timer_id, int duration, int period);

int sm_clear_event_timer(state_machine_t *sm, int timer_id);

int sm_dispatch_event(state_machine_t *sm, void *msg);

void sm_init(state_machine_t *sm, core_mq_t *msgq, void *msg, void *custom_ctx, const struct smf_state *hsm);

#endif // SM_H
