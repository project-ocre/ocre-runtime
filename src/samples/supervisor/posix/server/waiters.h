/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>

struct waiter;

// struct waiter *waiter_get_or_new(struct ocre_container *container);
// int waiter_add_client(struct waiter *waiter, int socket);

int container_waiter_add_client(struct ocre_container *container, int socket);
