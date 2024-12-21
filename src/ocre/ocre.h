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
#else
#define OCRE_LOG_LEVEL LOG_LEVEL_WRN
#endif

#endif