/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RNG_SENSOR_H
#define RNG_SENSOR_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif
#define SENSOR_CHAN_CUSTOM 12
int rng_sensor_init(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* RNG_SENSOR_H */