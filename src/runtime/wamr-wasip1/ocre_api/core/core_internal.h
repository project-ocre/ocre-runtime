/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_CORE_INTERNAL_H
#define OCRE_CORE_INTERNAL_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <time.h>
#include <errno.h>

#include <ocre/platform/config.h>

// Constants

/**
 * @brief Sets the value of argc for internal use.
 *
 * This function stores the argc value passed from main
 * so that other functions in the module can access it
 * without relying on global variables.
 *
 * @param argc The argument count from main.
 */
void set_argc(int argc);

/**
 * @brief Application version string.
 */
#ifndef APP_VERSION_STRING
#define APP_VERSION_STRING "0.0.0-dev"
#endif /* APP_VERSION_STRING */

/**
 * @brief Structure representing a thread in the Ocre runtime.
 */
struct core_thread {
	pthread_t tid;	       /*!< POSIX thread identifier */
	void *stack;	       /*!< Pointer to thread stack memory */
	size_t stack_size;     /*!< Size of the thread stack */
	uint32_t user_options; /*!< User-defined options for the thread */
};

/**
 * @brief Structure representing a mutex in the Ocre runtime.
 */
struct core_mutex {
	pthread_mutex_t native_mutex; /*!< POSIX mutex */
};

/**
 * @brief Structure representing a message queue in the Ocre runtime.
 */
struct core_mq {
	mqd_t msgq; /*!< POSIX message queue descriptor */
};

/**
 * @brief Timer callback function type.
 *
 * @param user_data Pointer to user data passed to the callback.
 */
typedef void (*core_timer_callback_t)(void *user_data);

/**
 * @brief Structure representing a timer in the Ocre runtime.
 */
struct core_timer {
	timer_t timerid;	  /*!< POSIX timer identifier */
	core_timer_callback_t cb; /*!< Timer callback function */
	void *user_data;	  /*!< User data for the callback */
};

/* Generic singly-linked list iteration macros */
#define CORE_SLIST_CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#define CORE_SLIST_FOR_EACH_CONTAINER_SAFE(list, var, tmp, member)                                                     \
	for (var = (list)->head ? CORE_SLIST_CONTAINER_OF((list)->head, __typeof__(*var), member) : NULL,              \
	    tmp = var ? (var->member.next ? CORE_SLIST_CONTAINER_OF(var->member.next, __typeof__(*var), member)        \
					  : NULL)                                                                      \
		      : NULL;                                                                                          \
	     var; var = tmp,                                                                                           \
	    tmp = tmp ? (tmp->member.next ? CORE_SLIST_CONTAINER_OF(tmp->member.next, __typeof__(*var), member)        \
					  : NULL)                                                                      \
		      : NULL)

#define CORE_SLIST_FOR_EACH_CONTAINER(list, var, member)                                                               \
	for (var = (list)->head ? CORE_SLIST_CONTAINER_OF((list)->head, __typeof__(*var), member) : NULL; var;         \
	     var = var->member.next ? CORE_SLIST_CONTAINER_OF(var->member.next, __typeof__(*var), member) : NULL)

/**
 * @brief Structure representing a node in a singly-linked list.
 */
typedef struct core_snode {
	struct core_snode *next; /*!< Pointer to the next node in the list */
} core_snode_t;

/**
 * @brief Structure representing a singly-linked list for POSIX platform.
 */
typedef struct {
	core_snode_t *head; /*!< Pointer to the first node in the list */
	core_snode_t *tail; /*!< Pointer to the last node in the list */
} core_slist_t;

/**
 * @brief Spinlock type for POSIX platform (simulated using mutex).
 */
typedef struct {
	pthread_mutex_t mutex; /*!< POSIX mutex for spinlock simulation */
} core_spinlock_t;

/**
 * @brief Spinlock key type for POSIX platform.
 */
typedef int core_spinlock_key_t;

/**
 * @brief Generic event queue structure for POSIX platform.
 *
 * A thread-safe circular buffer implementation that can store
 * any type of data items with configurable size and capacity.
 */
typedef struct {
	void *buffer;	       /*!< Dynamically allocated buffer for queue items */
	size_t item_size;      /*!< Size of each individual item in bytes */
	size_t max_items;      /*!< Maximum number of items the queue can hold */
	size_t count;	       /*!< Current number of items in the queue */
	size_t head;	       /*!< Index of the next item to be read */
	size_t tail;	       /*!< Index where the next item will be written */
	pthread_mutex_t mutex; /*!< Mutex for thread-safe access */
	pthread_cond_t cond;   /*!< Condition variable for signaling */
} core_eventq_t;

#endif /* OCRE_CORE_INTERNAL_H */
