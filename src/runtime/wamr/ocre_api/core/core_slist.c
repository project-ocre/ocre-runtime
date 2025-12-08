/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include "core_external.h"

void core_slist_init(core_slist_t *list)
{
	list->head = NULL;
	list->tail = NULL;
}

void core_slist_append(core_slist_t *list, core_snode_t *node)
{
	node->next = NULL;
	if (list->tail) {
		list->tail->next = node;
	} else {
		list->head = node;
	}
	list->tail = node;
}

void core_slist_remove(core_slist_t *list, core_snode_t *prev, core_snode_t *node)
{
	if (prev) {
		prev->next = node->next;
	} else {
		list->head = node->next;
	}
	if (list->tail == node) {
		list->tail = prev;
	}
}
