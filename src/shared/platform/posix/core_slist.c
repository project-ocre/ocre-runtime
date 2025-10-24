#include "ocre_core_external.h"

void posix_slist_init(posix_slist_t *list) {
    list->head = NULL;
    list->tail = NULL;
}

void posix_slist_append(posix_slist_t *list, posix_snode_t *node) {
    node->next = NULL;
    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
}

void posix_slist_remove(posix_slist_t *list, posix_snode_t *prev, posix_snode_t *node) {
    if (prev) {
        prev->next = node->next;
    } else {
        list->head = node->next;
    }
    if (list->tail == node) {
        list->tail = prev;
    }
}
