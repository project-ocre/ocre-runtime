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
#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include <smf/smf.h>

// Config macros
#define CONFIG_OCRE_CONTAINER_MESSAGING   /*!< Enable container messaging support */
#define CONFIG_OCRE_NETWORKING            /*!< Enable networking support */
#define CONFIG_OCRE_CONTAINER_FILESYSTEM
#define CONFIG_OCRE_CONTAINER_WAMR_TERMINATION
#define CONFIG_OCRE_TIMER

// Base paths for the application
#define OCRE_BASE_PATH "./ocre_data"           /*!< Base directory for Ocre resources */

#define APP_RESOURCE_PATH     OCRE_BASE_PATH "/images"     /*!< Path to container images */
#define PACKAGE_BASE_PATH     OCRE_BASE_PATH "/manifests"  /*!< Path to package manifests */
#define CONFIG_PATH           OCRE_BASE_PATH "/config"     /*!< Path to configuration files */

/**
 * @brief Path for container filesystem root
 */
#define CONTAINER_FS_PATH     OCRE_BASE_PATH "/cfs"

/**
 * @brief Ignore Zephyr's log module registration on POSIX.
 */
#define LOG_MODULE_REGISTER(name, level)
#define LOG_MODULE_DECLARE(name, level)


/*
 * @brief Log level priority definitions (highest to lowest)
 */
#define APP_LOG_LEVEL_ERR  1
#define APP_LOG_LEVEL_WRN  2
#define APP_LOG_LEVEL_INF  3
#define APP_LOG_LEVEL_DBG  4

/*
 * @brief Determine the current log level based on CONFIG defines
 * Priority: CONFIG_LOG_LVL_ERR > CONFIG_LOG_LVL_WRN > CONFIG_LOG_LVL_INF > CONFIG_LOG_LVL_DBG
 * If none specified, default to INFO level
 */
#if defined(CONFIG_LOG_LVL_ERR)
    #define APP_CURRENT_LOG_LEVEL APP_LOG_LEVEL_ERR
#elif defined(CONFIG_LOG_LVL_WRN)
    #define APP_CURRENT_LOG_LEVEL APP_LOG_LEVEL_WRN
#elif defined(CONFIG_LOG_LVL_INF)
    #define APP_CURRENT_LOG_LEVEL APP_LOG_LEVEL_INF
#elif defined(CONFIG_LOG_LVL_DBG)
    #define APP_CURRENT_LOG_LEVEL APP_LOG_LEVEL_DBG
#else
    #define APP_CURRENT_LOG_LEVEL APP_LOG_LEVEL_INF  /* Default to INFO level */
#endif

/**
 * @brief Log an error message (always shown if ERR level or higher).
 */
#if APP_CURRENT_LOG_LEVEL >= APP_LOG_LEVEL_ERR
    #define LOG_ERR(fmt, ...)   printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_ERR(fmt, ...)   do { } while(0)
#endif

/**
 * @brief Log a warning message (shown if WRN level or higher).
 */
#if APP_CURRENT_LOG_LEVEL >= APP_LOG_LEVEL_WRN
    #define LOG_WRN(fmt, ...)   printf("[WARNING] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_WRN(fmt, ...)   do { } while(0)
#endif

/**
 * @brief Log an informational message (shown if INF level or higher).
 */
#if APP_CURRENT_LOG_LEVEL >= APP_LOG_LEVEL_INF
    #define LOG_INF(fmt, ...)   printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_INF(fmt, ...)   do { } while(0)
#endif

/**
 * @brief Log a debug message (shown only if DBG level).
 */
#if APP_CURRENT_LOG_LEVEL >= APP_LOG_LEVEL_DBG
    #define LOG_DBG(fmt, ...)   printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_DBG(fmt, ...)   do { } while(0)
#endif

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
 * @brief Maximum length for SHA256 string representations.
 */
#define OCRE_SHA256_LEN 128

/**
 * @brief Maximum number of containers supported.
 */
#define CONFIG_MAX_CONTAINERS 10

/**
 * @brief Default stack size for container supervisor threads (in bytes).
 */
#define OCRE_CS_THREAD_STACK_SIZE (1024 * 1024)

/**
 * @brief Application version string.
 */
#ifndef APP_VERSION_STRING
#define APP_VERSION_STRING           "0.0.0-dev"
#endif /* APP_VERSION_STRING */

/**
 * @brief Default heap buffer size for WAMR (in bytes).
 */
#define CONFIG_OCRE_WAMR_HEAP_BUFFER_SIZE           512000

/**
 * @brief Default heap size for a container (in bytes).
 */
#define CONFIG_OCRE_CONTAINER_DEFAULT_HEAP_SIZE     4096

/**
 * @brief Default stack size for a container (in bytes).
 */
#define CONFIG_OCRE_CONTAINER_DEFAULT_STACK_SIZE    4096 * 16

/**
 * @brief Default stack size for container threads (in bytes).
 */
#define CONTAINER_THREAD_STACK_SIZE 1024 * 1024

/**
 * @brief Structure representing a thread in the Ocre runtime.
 */
struct core_thread {
    pthread_t tid;         /*!< POSIX thread identifier */
    void *stack;           /*!< Pointer to thread stack memory */
    size_t stack_size;     /*!< Size of the thread stack */
    uint32_t user_options; /*!< User-defined options for the thread */
};

/**
 * @brief Structure representing a mutex in the Ocre runtime.
 */
struct core_mutex {
    pthread_mutex_t native_mutex; /*!< POSIX mutex */
};

#define MQ_DEFAULT_PRIO 0 /*!< Default message queue priority */

/**
 * @brief Structure representing a message queue in the Ocre runtime.
 */
struct core_mq {
    mqd_t msgq;              /*!< POSIX message queue descriptor */
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
    timer_t timerid;                /*!< POSIX timer identifier */
    core_timer_callback_t cb;       /*!< Timer callback function */
    void *user_data;                /*!< User data for the callback */
};

/* Generic singly-linked list iteration macros */
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define CORE_SLIST_FOR_EACH_CONTAINER_SAFE(list, var, tmp, member) \
    for (var = (list)->head ? CONTAINER_OF((list)->head, __typeof__(*var), member) : NULL, \
         tmp = var ? (var->member.next ? CONTAINER_OF(var->member.next, __typeof__(*var), member) : NULL) : NULL; \
         var; \
         var = tmp, tmp = tmp ? (tmp->member.next ? CONTAINER_OF(tmp->member.next, __typeof__(*var), member) : NULL) : NULL)

#define CORE_SLIST_FOR_EACH_CONTAINER(list, var, member) \
    for (var = (list)->head ? CONTAINER_OF((list)->head, __typeof__(*var), member) : NULL; \
         var; \
         var = var->member.next ? CONTAINER_OF(var->member.next, __typeof__(*var), member) : NULL)

/**
 * @brief Structure representing a node in a singly-linked list.
 */
typedef struct core_snode {
    struct core_snode *next;       /*!< Pointer to the next node in the list */
} core_snode_t;

/**
 * @brief Structure representing a singly-linked list for POSIX platform.
 */
typedef struct {
    core_snode_t *head;            /*!< Pointer to the first node in the list */
    core_snode_t *tail;            /*!< Pointer to the last node in the list */
} core_slist_t;

/**
 * @brief Initialize a singly-linked list.
 *
 * @param list Pointer to the list to initialize.
 */
void core_slist_init(core_slist_t *list);

/**
 * @brief Append a node to the end of a singly-linked list.
 *
 * @param list Pointer to the list to append to.
 * @param node Pointer to the node to append.
 */
void core_slist_append(core_slist_t *list, core_snode_t *node);

/**
 * @brief Remove a node from a singly-linked list.
 *
 * @param list Pointer to the list to remove from.
 * @param prev Pointer to the previous node (or NULL if removing head).
 * @param node Pointer to the node to remove.
 */
void core_slist_remove(core_slist_t *list, core_snode_t *prev, core_snode_t *node);

/**
 * @brief Spinlock type for POSIX platform (simulated using mutex).
 */
typedef struct {
    pthread_mutex_t mutex;          /*!< POSIX mutex for spinlock simulation */
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
    void *buffer;                   /*!< Dynamically allocated buffer for queue items */
    size_t item_size;               /*!< Size of each individual item in bytes */
    size_t max_items;               /*!< Maximum number of items the queue can hold */
    size_t count;                   /*!< Current number of items in the queue */
    size_t head;                    /*!< Index of the next item to be read */
    size_t tail;                    /*!< Index where the next item will be written */
    pthread_mutex_t mutex;          /*!< Mutex for thread-safe access */
    pthread_cond_t cond;            /*!< Condition variable for signaling */
} core_eventq_t;

#endif /* OCRE_CORE_INTERNAL_H */
