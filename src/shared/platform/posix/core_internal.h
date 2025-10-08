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
#include <pthread.h>
#include <mqueue.h>
#include <time.h>

#include <smf/smf.h>

// Config macros
//#define CONFIG_OCRE_CONTAINER_MESSAGING   /*!< Enable container messaging support */
#define CONFIG_OCRE_NETWORKING            /*!< Enable networking support */
#define CONFIG_OCRE_CONTAINER_FILESYSTEM
#define CONFIG_OCRE_CONTAINER_WAMR_TERMINATION

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


/**
 * @brief Log a debug message.
 */
#define LOG_DBG(fmt, ...)   printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

/**
 * @brief Log an error message.
 */
#define LOG_ERR(fmt, ...)   printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

/**
 * @brief Log a warning message.
 */
#define LOG_WRN(fmt, ...)   printf("[WARNING] " fmt "\n", ##__VA_ARGS__)

/**
 * @brief Log an informational message.
 */
#define LOG_INF(fmt, ...)   printf("[INFO] " fmt "\n", ##__VA_ARGS__)

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
#define APP_VERSION_STRING           "0.0.0-dev"

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

#endif /* OCRE_CORE_INTERNAL_H */
