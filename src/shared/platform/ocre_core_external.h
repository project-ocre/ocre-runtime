/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_CORE_EXTERNAL_H
#define OCRE_CORE_EXTERNAL_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __ZEPHYR__
#include "zephyr/core_internal.h"
#else
#include "posix/core_internal.h"
#endif

#define OCRE_MODULE_NAME_LEN 16

/**
 * @brief Initialize application storage subsystem.
 */
void ocre_app_storage_init();

/**
 * @brief Thread function type definition.
 *
 * @param arg Pointer to user data passed to the thread.
 */
typedef void (*core_thread_func_t)(void *arg);

typedef struct core_thread core_thread_t;

/**
 * @brief Create a new thread.
 *
 * @param thread Pointer to thread structure to initialize.
 * @param func Thread entry function.
 * @param arg Argument to pass to the thread function.
 * @param name Name of the thread.
 * @param stack_size Stack size in bytes.
 * @param priority Thread priority.
 * @return 0 on success, negative value on error.
 */
int core_thread_create(core_thread_t *thread, core_thread_func_t func, void *arg, const char *name, size_t stack_size, int priority);

/**
 * @brief Destroy a thread and release its resources.
 *
 * @param thread Pointer to the thread structure.
 */
void core_thread_destroy(core_thread_t *thread);

typedef struct core_mutex core_mutex_t;

/**
 * @brief Initialize a mutex.
 *
 * @param mutex Pointer to the mutex structure.
 * @return 0 on success, negative value on error.
 */
int core_mutex_init(core_mutex_t *mutex);

/**
 * @brief Destroy a mutex and release its resources.
 *
 * @param mutex Pointer to the mutex structure.
 * @return 0 on success, negative value on error.
 */
int core_mutex_destroy(core_mutex_t *mutex);

/**
 * @brief Lock a mutex.
 *
 * @param mutex Pointer to the mutex structure.
 * @return 0 on success, negative value on error.
 */
int core_mutex_lock(core_mutex_t *mutex);

/**
 * @brief Unlock a mutex.
 *
 * @param mutex Pointer to the mutex structure.
 * @return 0 on success, negative value on error.
 */
int core_mutex_unlock(core_mutex_t *mutex);

typedef struct core_mq core_mq_t;

/**
 * @brief Initialize a message queue.
 *
 * @param mq Pointer to the message queue structure.
 * @param name Name of the message queue.
 * @param msg_size Size of each message in bytes.
 * @param max_msgs Maximum number of messages in the queue.
 * @return 0 on success, negative value on error.
 */
int core_mq_init(core_mq_t *mq, const char *name, size_t msg_size, uint32_t max_msgs);

/**
 * @brief Send a message to the queue.
 *
 * @param mq Pointer to the message queue.
 * @param data Pointer to the message data.
 * @param msg_len Length of the message in bytes.
 * @return 0 on success, negative value on error.
 */
int core_mq_send(core_mq_t *mq, const void *data, size_t msg_len);

/**
 * @brief Receive a message from the queue.
 *
 * @param mq Pointer to the message queue.
 * @param data Pointer to the buffer to store the received message.
 * @return 0 on success, negative value on error.
 */
int core_mq_recv(core_mq_t *mq, void *data);

/**
 * @brief Sleep for a specified number of milliseconds.
 *
 * @param milliseconds Number of milliseconds to sleep.
 */
void core_sleep_ms(int milliseconds);

/**
 * @brief Allocate memory.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *core_malloc(size_t size);

/**
 * @brief Free previously allocated memory.
 *
 * @param ptr Pointer to memory to free.
 */
void core_free(void *ptr);

/**
 * @brief Yield the current thread's execution.
 */
void core_yield(void);

typedef struct core_timer core_timer_t;

/**
 * @brief Initialize a timer.
 *
 * @param timer Pointer to the timer structure.
 * @param cb Callback function to call when the timer expires.
 * @param user_data Pointer to user data for the callback.
 * @return 0 on success, negative value on error.
 */
int core_timer_init(core_timer_t *timer, core_timer_callback_t cb, void *user_data);

/**
 * @brief Start a timer.
 *
 * @param timer Pointer to the timer structure.
 * @param timeout_ms Timeout in milliseconds before the timer expires.
 * @param period_ms Period in milliseconds for periodic timers (0 for one-shot).
 * @return 0 on success, negative value on error.
 */
int core_timer_start(core_timer_t *timer, int timeout_ms, int period_ms);

/**
 * @brief Stop a timer.
 *
 * @param timer Pointer to the timer structure.
 * @return 0 on success, negative value on error.
 */
int core_timer_stop(core_timer_t *timer);

/**
 * @brief Get file status (size).
 *
 * @param path Path to the file.
 * @param size Pointer to store the file size.
 * @return 0 on success, negative value on error.
 */
int core_filestat(const char *path, size_t *size);

/**
 * @brief Open a file for reading.
 *
 * @param path Path to the file.
 * @param handle Pointer to store the file handle.
 * @return 0 on success, negative value on error.
 */
int core_fileopen(const char *path, void **handle);

/**
 * @brief Read data from a file.
 *
 * @param handle File handle.
 * @param buffer Buffer to store the read data.
 * @param size Number of bytes to read.
 * @return 0 on success, negative value on error.
 */
int core_fileread(void *handle, void *buffer, size_t size);

/**
 * @brief Close a file.
 *
 * @param handle File handle.
 * @return 0 on success, negative value on error.
 */
int core_fileclose(void *handle);

/**
 * @brief Construct a file path for a container image.
 *
 * @param path Buffer to store the constructed path.
 * @param len Length of the buffer.
 * @param name Name or identifier for the file.
 * @return 0 on success, negative value on error.
 */
int core_construct_filepath(char *path, size_t len, char *name);

/**
 * @brief Get system uptime in milliseconds.
 *
 * @return System uptime in milliseconds.
 */
uint32_t core_uptime_get(void);

/**
 * @brief Lock a spinlock and return the interrupt key.
 *
 * @param lock Pointer to the spinlock structure.
 * @return Interrupt key to be used with unlock.
 */
core_spinlock_key_t core_spinlock_lock(core_spinlock_t *lock);

/**
 * @brief Unlock a spinlock using the interrupt key.
 *
 * @param lock Pointer to the spinlock structure.
 * @param key Interrupt key returned from lock operation.
 */
void core_spinlock_unlock(core_spinlock_t *lock, core_spinlock_key_t key);

#endif /* OCRE_CORE_EXTERNAL_H */
