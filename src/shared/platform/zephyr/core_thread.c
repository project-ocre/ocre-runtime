/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocre_core_external.h"
#include <zephyr/kernel.h>
#include <string.h>


static void thread_entry(void *arg1, void *arg2, void *arg3) {
    core_thread_func_t func = (core_thread_func_t)arg2;
    void *user_arg = arg3;
    func(user_arg);
}

int core_thread_create(core_thread_t *thread, core_thread_func_t func, void *arg, const char *name, size_t stack_size, int priority) {
    if (!thread || !func) return -1;
    thread->stack = k_malloc(K_THREAD_STACK_LEN(stack_size));
    if (!thread->stack) return -1;
    thread->tid = k_thread_create(&thread->thread, thread->stack, stack_size,
                                  thread_entry, NULL, (void *)func, arg,
                                  priority, 0, K_NO_WAIT);
    if (name)
        k_thread_name_set(thread->tid, name);
    return thread->tid ? 0 : -1;
}

void core_thread_destroy(core_thread_t *thread) {
    if (!thread) return;
    k_thread_abort(thread->tid);
    k_free(thread->stack);
}
