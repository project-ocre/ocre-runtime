/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include "ocre_core_external.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "sm.h"
LOG_MODULE_REGISTER(state_machine, OCRE_LOG_LEVEL);

void sm_init(state_machine_t *sm, core_mq_t *msgq, void *msg, void *custom_ctx, const struct smf_state *hsm) {
    sm->hsm = hsm;
    sm->msgq = msgq;
    sm->ctx.event.msg = msg;
    sm->ctx.custom_ctx = custom_ctx;
}

int sm_run(state_machine_t *sm, int initial_state) {
    int ret;

    smf_set_initial(SMF_CTX(&sm->ctx), &sm->hsm[initial_state]);

    while (true) {
        // Wait for a message from the queue
        core_mq_recv(sm->msgq, sm->ctx.event.msg);
        sm->ctx.event.handled = false;

        ret = smf_run_state(SMF_CTX(&sm->ctx));

        if (ret) {
            LOG_ERR("State machine error: %d", ret);
            break;
        }

        if (!sm->ctx.event.handled) {
            LOG_ERR("Unhandled event");
        }

        // Yield the current thread to allow the queue events to be processed
        core_yield();
    }

    return ret;
}

int sm_transition(state_machine_t *sm, int target_state) {
    if (!sm->hsm) {
        LOG_ERR("State machine has not been initialized");
        return -EINVAL;
    }

    smf_set_state(SMF_CTX(&sm->ctx), &sm->hsm[target_state]);

    return 0;
}

int sm_init_event_timer(state_machine_t *sm, int timer_id, void *timer_cb) {
    if (timer_id < 0 || timer_id >= MAX_TIMERS) {
        LOG_ERR("Invalid timer id: %d", timer_id);
        return -EINVAL;
    }

    core_timer_init(&sm->timers[timer_id], timer_cb, NULL);
    return 0;
}

int sm_set_event_timer(state_machine_t *sm, int timer_id, int duration, int period) {
    if (timer_id < 0 || timer_id >= MAX_TIMERS) {
        LOG_ERR("Invalid timer id: %d", timer_id);
        return -EINVAL;
    }

    core_timer_start(&sm->timers[timer_id], duration, period);
    return 0;
}

int sm_clear_event_timer(state_machine_t *sm, int timer_id) {
    if (timer_id < 0 || timer_id >= MAX_TIMERS) {
        LOG_ERR("Invalid timer id: %d", timer_id);
        return -EINVAL;
    }

    core_timer_stop(&sm->timers[timer_id]);
    return 0;
}
