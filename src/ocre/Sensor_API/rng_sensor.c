#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sensor.h>
#include <zephyr/sys/random/rand32.h>
#include "rng_sensor.h"

/* Define the sensor channels */
static enum sensor_channel rng_sensor_channels[] = {
    SENSOR_CHAN_CUSTOM + 1, /* Define a custom channel */
};

/* Define the sensor data structure */
struct rng_sensor_data
{
    struct sensor_value value;
};

/* Define the sensor driver API functions */

/* Function to get sensor data */
static int rng_sensor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    struct rng_sensor_data *data = dev->data;

    /* Generate a random 32-bit number */
    uint32_t random_number;
    sys_rand32_get(&random_number);

    /* Store the random number in sensor_value */
    data->value.val1 = random_number; /* Integer part */
    data->value.val2 = 0;             /* Fractional part */

    return 0;
}

/* Function to retrieve sensor data */
static int rng_sensor_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
    struct rng_sensor_data *data = dev->data;

    if (chan != SENSOR_CHAN_CUSTOM + 1)
    {
        return -ENOTSUP;
    }

    *val = data->value;

    return 0;
}

/* Define the sensor driver API */
static const struct sensor_driver_api rng_sensor_api = {
    .sample_fetch = rng_sensor_sample_fetch,
    .channel_get = rng_sensor_channel_get,
};

/* Initialization function */
int rng_sensor_init(const struct device *dev)
{
    /* Initialization code if needed */
    return 0;
}

/* Define the sensor device */
static struct rng_sensor_data rng_sensor_data;

DEVICE_DT_DEFINE(DT_DRV_INST(0),
                 rng_sensor_init,
                 NULL,
                 &rng_sensor_data,
                 NULL,
                 POST_KERNEL,
                 CONFIG_SENSOR_INIT_PRIORITY,
                 &rng_sensor_api);
