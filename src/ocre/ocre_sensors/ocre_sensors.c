/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sensor.h>
#include <zephyr/kernel.h>

#include <ocre/container_runtime.h>
#include "ocre_sensors.h"

ocre_sensors_status_t ocre_sensors_init()
{
    // add ocre_sensors initialization
    return SENSOR_API_STATUS_INITIALIZED;
}

ocre_sensors_status_t ocre_sensors_discover_sensors(ocre_sensor_t *sensors, int *sensors_count)
{
    const struct device *dev;
    int sensor_index = 0;
    struct sensor_value value;
    int channel_count = 0;

    Z_DEVICE_FOREACH(sensor, dev)
    {
        if (!device_is_ready(dev))
        {
            LOG_INF("Sensor device %s is not ready\n", dev->name);
            continue;
        }

        sensors[sensor_index].handle.id = sensor_index;
        sensors[sensor_index].handle.device_name = dev->name; // Set device name from the device tree

        if (sensor_channel_get(dev, SENSOR_CHAN_ALL, &value) == 0)
        {
            if (sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, &value) == 0)
            {
                sensors[sensor_index].channels[channel_count++] = SENSOR_CHANNEL_ACCELERATION;
            }
            if (sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, &value) == 0)
            {
                sensors[sensor_index].channels[channel_count++] = SENSOR_CHANNEL_GYRO;
            }
            if (sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, &value) == 0)
            {
                sensors[sensor_index].channels[channel_count++] = SENSOR_CHANNEL_MAGNETIC_FIELD;
            }
            if (sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &value) == 0)
            {
                sensors[sensor_index].channels[channel_count++] = SENSOR_CHANNEL_LIGHT;
            }
            if (sensor_channel_get(dev, SENSOR_CHAN_PRESS, &value) == 0)
            {
                sensors[sensor_index].channels[channel_count++] = SENSOR_CHANNEL_PRESSURE;
            }
            if (sensor_channel_get(dev, SENSOR_CHAN_GAUGE_TEMP, &value) == 0)
            {
                sensors[sensor_index].channels[channel_count++] = SENSOR_CHANNEL_TEMPERATURE;
            }
        }

        sensors[sensor_index].num_channels = channel_count;

        sensor_index++;
    }

    *sensors_count = sensor_index;

    // no sensors found
    if (sensor_index == 0)
    {
        return SENSOR_API_STATUS_ERROR;
    }
    return SENSOR_API_STATUS_INITIALIZED;
}

ocre_sensors_status_t ocre_sensors_open_channel(ocre_sensor_handle_t *sensor_handle)
{
    const struct device *dev = device_get_binding(sensor_handle->device_name);

    if (!dev)
    {
        LOG_INF("Failed to open sensor channel for device %s", sensor_handle->device_name);
        return SENSOR_API_STATUS_ERROR;
    }

    LOG_INF("Sensor device %s is ready and opened", sensor_handle->device_name);
    sensor_handle->sensor_device = dev;
    return SENSOR_API_STATUS_CHANNEL_OPENED;
}

ocre_sensors_sample_t sensor_read_sample(ocre_sensor_handle_t *sensor_handle)
{
    ocre_sensors_sample_t sample;
    const struct device *dev = device_get_binding(sensor_handle->device_name);

    if (!dev)
    {
        LOG_INF("Failed to read from sensor %d\n", sensor_handle->id);
        return sample; // return empty sample on error
    }

    if (sensor_sample_fetch(dev) < 0)
    {
        LOG_INF("Failed to fetch sensor sample\n");
        return sample; // return empty sample on error
    }

    struct sensor_value sensor_val;
    if (sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, &sensor_val) == 0)
    {
        sample.value.integer = sensor_val.val1;
        sample.value.floating = sensor_val.val2;
    }

    return sample;
}

ocre_sensor_value_t sensor_get_channel(ocre_sensors_sample_t sample, sensor_channel_t channel)
{
    ocre_sensor_value_t value;
    value.integer = sample.value.integer;
    value.floating = sample.value.floating;
    return value;
}

ocre_sensors_status_t ocre_sensors_set_trigger(ocre_sensor_handle_t sensor_handle, sensor_channel_t channel, enum sensor_trigger_type trigger_type, ocre_sensor_trigger_cb callback, int *subscription_id)
{
    const struct device *dev = device_get_binding(sensor_handle.device_name);

    if (!dev)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    struct sensor_trigger trig = {
        .type = trigger_type,
        .chan = channel,
    };

    if (sensor_trigger_set(dev, &trig, (sensor_trigger_handler_t)callback) != 0)
    {
        LOG_INF("Failed to set sensor trigger");
        return SENSOR_API_STATUS_ERROR;
    }

    *subscription_id = sensor_handle->id;

    return SENSOR_API_STATUS_OKEY;
}

ocre_sensors_status_t ocre_sensors_clear_trigger(ocre_sensor_handle_t sensor_handle, sensor_channel_t channel, int subscription_id)
{
    const struct device *dev = device_get_binding(sensor_handle.device_name);

    if (!dev)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    struct sensor_trigger trig = {
        .chan = channel,
    };

    if (sensor_trigger_set(dev, &trig, NULL) != 0)
    {
        LOG_INF("Failed to clear sensor trigger");
        return SENSOR_API_STATUS_ERROR;
    }

    return SENSOR_API_STATUS_OKEY;
}

ocre_sensors_status_t ocre_sensors_cleanup()
{
    return SENSOR_API_STATUS_UNINITIALIZED;
}

/* OCRE_SENSORS_C */
