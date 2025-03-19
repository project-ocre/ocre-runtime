/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_H
#define OCRE_H

#include <zephyr/logging/log.h>

#if CONFIG_OCRE_LOG_DEBUG
#define OCRE_LOG_LEVEL LOG_LEVEL_DBG
#elif CONFIG_OCRE_LOG_ERR
#define OCRE_LOG_LEVEL LOG_LEVEL_ERR
#elif CONFIG_OCRE_LOG_WARN
#define OCRE_LOG_LEVEL LOG_LEVEL_WRN
#elif CONFIG_OCRE_LOG_INF
#define OCRE_LOG_LEVEL LOG_LEVEL_INF
#else
#define OCRE_LOG_LEVEL LOG_LEVEL_NONE
#endif

#endif