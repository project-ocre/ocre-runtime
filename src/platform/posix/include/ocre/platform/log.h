/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "config.h"

#define LOG_MODULE_REGISTER(module, ...) static const char *const __ocre_log_module = #module
#define LOG_MODULE_DECLARE(module, ...)	 static const char *const __ocre_log_module = #module

extern int __ocre_log_level;

#if CONFIG_OCRE_LOG_LEVEL >= 1
#define LOG_ERR(fmt, ...)                                                                                              \
	do {                                                                                                           \
		if (__ocre_log_level >= 1)                                                                             \
			fprintf(stderr, "<err> %s: " fmt "\n", __ocre_log_module, ##__VA_ARGS__);                      \
	} while (0)
#else
#define LOG_ERR(fmt, ...)                                                                                              \
	do {                                                                                                           \
	} while (0)
#endif

#if CONFIG_OCRE_LOG_LEVEL >= 2
#define LOG_WRN(fmt, ...)                                                                                              \
	do {                                                                                                           \
		if (__ocre_log_level >= 2)                                                                             \
			fprintf(stderr, "<wrn> %s: " fmt "\n", __ocre_log_module, ##__VA_ARGS__);                      \
	} while (0)
#else
#define LOG_WRN(fmt, ...)                                                                                              \
	do {                                                                                                           \
	} while (0)
#endif

#if CONFIG_OCRE_LOG_LEVEL >= 3
#define LOG_INF(fmt, ...)                                                                                              \
	do {                                                                                                           \
		if (__ocre_log_level >= 3)                                                                             \
			fprintf(stderr, "<inf> %s: " fmt "\n", __ocre_log_module, ##__VA_ARGS__);                      \
	} while (0)
#else
#define LOG_INF(fmt, ...)                                                                                              \
	do {                                                                                                           \
	} while (0)
#endif

#if CONFIG_OCRE_LOG_LEVEL >= 4
#define LOG_DBG(fmt, ...)                                                                                              \
	do {                                                                                                           \
		if (__ocre_log_level >= 4)                                                                             \
			fprintf(stderr, "<dbg> %s: " fmt "\n", __ocre_log_module, ##__VA_ARGS__);                      \
	} while (0)
#else
#define LOG_DBG(fmt, ...)                                                                                              \
	do {                                                                                                           \
	} while (0)
#endif
