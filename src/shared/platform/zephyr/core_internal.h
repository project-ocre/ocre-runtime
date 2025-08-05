/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_CORE_INTERNAL_H
#define OCRE_CORE_INTERNAL_H

#include <zephyr/smf.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/kernel/mm.h>

#include <version.h>
#include <app_version.h>
#include <autoconf.h>

// clang-format off

/**
 * @brief Filesystem mount point for Ocre resources.
 */
#define FS_MOUNT_POINT "/lfs"

/**
 * @brief Base directory for Ocre resources.
 */
#define OCRE_BASE_PATH FS_MOUNT_POINT "/ocre"

/**
 * @brief Path to container images.
 */
#define APP_RESOURCE_PATH     OCRE_BASE_PATH "/images"

/**
 * @brief Path to package manifests.
 */
#define PACKAGE_BASE_PATH     OCRE_BASE_PATH "/manifests"

/**
 * @brief Path to configuration files.
 */
#define CONFIG_PATH           OCRE_BASE_PATH "/config/"

/**
 * @brief Path for container filesystem root
 */
#define CONTAINER_FS_PATH     OCRE_BASE_PATH "/cfs"

// Constants

/**
 * @brief Default stack size for container supervisor threads (in bytes).
 */
#define OCRE_CS_THREAD_STACK_SIZE 4096

/**
 * @brief Default stack size for container threads (in bytes).
 */
#define CONTAINER_THREAD_STACK_SIZE 8192

/**
 * @brief Default stack size for event threads (in bytes).
 */
#define EVENT_THREAD_STACK_SIZE 2048

/**
 * @brief Maximum length for SHA256 string representations.
 */
#define OCRE_SHA256_LEN 128

/**
 * @brief Structure representing a thread in the Ocre runtime (Zephyr).
 */
struct core_thread {
    struct k_thread thread;         /*!< Zephyr thread structure */
    k_tid_t tid;                    /*!< Zephyr thread identifier */
    k_thread_stack_t *stack;        /*!< Pointer to thread stack memory */
    size_t stack_size;              /*!< Size of the thread stack */
    uint32_t user_options;          /*!< User-defined options for the thread */
};

/**
 * @brief Structure representing a mutex in the Ocre runtime (Zephyr).
 */
struct core_mutex {
    struct k_mutex native_mutex;    /*!< Zephyr native mutex */
};

/**
 * @brief Default timeout value for message queue operations.
 */
#define MQ_DEFAULT_TIMEOUT K_NO_WAIT

/**
 * @brief Structure representing a message queue in the Ocre runtime (Zephyr).
 */
struct core_mq {
    struct k_msgq msgq;             /*!< Zephyr message queue */
    char __aligned(4) *msgq_buffer; /*!< Message queue buffer (aligned) */
};

/**
 * @brief Timer callback function type.
 *
 * @param user_data Pointer to user data passed to the callback.
 */
typedef void (*core_timer_callback_t)(void *user_data);

/**
 * @brief Structure representing a timer in the Ocre runtime (Zephyr).
 */
struct core_timer {
    struct k_timer timer;           /*!< Zephyr timer structure */
    core_timer_callback_t cb;       /*!< Timer callback function */
    void *user_data;                /*!< User data for the callback */
};

#endif
