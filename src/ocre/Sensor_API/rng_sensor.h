#ifndef RNG_SENSOR_H
#define RNG_SENSOR_H

#include <zephyr/device.h>
#include <zephyr/sensor.h>

#ifdef __cplusplus
extern "C"
{
#endif
#define SENSOR_CHAN_CUSTOM 1
    int rng_sensor_init(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* RNG_SENSOR_H */