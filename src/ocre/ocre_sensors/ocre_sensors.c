/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <ocre/ocre.h>
LOG_MODULE_DECLARE(ocre_sensors, OCRE_LOG_LEVEL);

#include <ocre/ocre_container_runtime/ocre_container_runtime.h>
#include "ocre_sensors.h"

#define MAX_SENSORS             16
#define MAX_CHANNELS_PER_SENSOR 8

typedef struct {
    const struct device *device;
    const char *name;
    int channels[MAX_CHANNELS_PER_SENSOR];
    int num_channels;
    bool in_use;
    int id;
} ocre_sensor_internal_t;

static ocre_sensor_internal_t sensors[MAX_SENSORS] = {0};
static int sensor_count = 0;

static const int possible_channels[] = {SENSOR_CHAN_ACCEL_XYZ, SENSOR_CHAN_GYRO_XYZ, SENSOR_CHAN_MAGN_XYZ,
                                        SENSOR_CHAN_LIGHT,     SENSOR_CHAN_PRESS,    SENSOR_CHAN_GAUGE_TEMP};

int ocre_sensors_init(wasm_exec_env_t exec_env, int unused) {
    memset(sensors, 0, sizeof(sensors));
    sensor_count = 0;
    return 0;
}

int ocre_sensors_discover(wasm_exec_env_t exec_env, int unused) {
    const struct device *dev = NULL;
    size_t device_count = z_device_get_all_static(&dev);

    if (!dev) {
        LOG_ERR("Device list is NULL. Possible memory corruption!");
        return -1;
    }

    if (device_count == 0) {
        LOG_ERR("No devices found");
        return -1;
    }

    LOG_INF("Total devices found: %zu", device_count);

    for (size_t i = 0; i < device_count && sensor_count < MAX_SENSORS; i++) {
        if (!&dev[i]) {
            LOG_ERR("Device index %zu is NULL, skipping!", i);
            continue;
        }

        if (!dev[i].name) {
            LOG_ERR("Device %zu has NULL name, skipping!", i);
            continue;
        }

        LOG_INF("Checking device index: %zu - Name: %s", i, dev[i].name);

        if (!device_is_ready(&dev[i])) {
            LOG_WRN("Device %s is not ready, skipping", dev[i].name);
            continue;
        }

        if (!dev[i].api) {
            LOG_WRN("Device %s has NULL API, skipping", dev[i].name);
            continue;
        }

        const struct sensor_driver_api *api = (const struct sensor_driver_api *)dev[i].api;
        if (!api || !api->channel_get) {
            LOG_WRN("Device %s does not support sensor API or channel_get, skipping", dev[i].name);
            continue;
        }

        struct sensor_value value = {0}; // Ensure proper allocation
        int channel_count = 0;

        sensors[sensor_count].device = &dev[i];
        sensors[sensor_count].name = dev[i].name;
        sensors[sensor_count].id = sensor_count;
        sensors[sensor_count].in_use = true;

        int max_channels = ocre_sensors_get_channel_count(exec_env, sensor_count);
        LOG_DBG("Device %s has %d total channels", dev[i].name, max_channels);

        for (int channel_idx = 0; channel_idx < max_channels; channel_idx++) {
            int channel = ocre_sensors_get_channel_type(exec_env, sensor_count, channel_idx);
            LOG_DBG("Testing channel %d (%d) on device %s", channel_idx, channel, dev[i].name);

            if (sensor_channel_get(&dev[i], channel, &value) == 0) {
                sensors[sensor_count].channels[channel_count++] = channel;
                LOG_INF("Device %s supports channel %d", dev[i].name, channel);
            } else {
                LOG_WRN("Device %s does not support channel %d", dev[i].name, channel);
            }
        }

        sensors[sensor_count].num_channels = channel_count;
        LOG_INF("Device %s has %d available channels", dev[i].name, channel_count);

        sensor_count++;
    }

    LOG_INF("Discovered %d sensors", sensor_count);
    return sensor_count;
}

int ocre_sensors_get_count(wasm_exec_env_t exec_env, int unused) {
    return sensor_count;
}

// Get information about a specific sensor
// Returns: 0 on success, -1 on error
int ocre_sensors_get_info(wasm_exec_env_t exec_env, int sensor_id, int info_type) {
    if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use) {
        return -1;
    }

    switch (info_type) {
        case 0: // Get sensor ID
            return sensors[sensor_id].id;
        case 1: // Get number of channels
            return sensors[sensor_id].num_channels;
        default:
            return -1;
    }
}

// Read sensor data
// channel_type corresponds to SENSOR_CHAN_* values
// Returns: sensor value * 1000 (fixed point) or -1 on error
int ocre_sensors_read(wasm_exec_env_t exec_env, int sensor_id, int channel_type) {
    if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use) {
        return -1;
    }

    const struct device *dev = sensors[sensor_id].device;
    struct sensor_value value;

    if (sensor_sample_fetch(dev) < 0) {
        return -1;
    }

    if (sensor_channel_get(dev, channel_type, &value) < 0) {
        return -1;
    }

    // Convert to fixed point: val * 1000
    return value.val1 * 1000 + value.val2 / 1000;
}

int ocre_sensors_get_channel_count(wasm_exec_env_t exec_env, int sensor_id) {
    if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use) {
        return -1;
    }
    return sensors[sensor_id].num_channels;
}

int ocre_sensors_get_channel_type(wasm_exec_env_t exec_env, int sensor_id, int channel_index) {
    if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use || channel_index < 0 ||
        channel_index >= sensors[sensor_id].num_channels) {
        return -1;
    }
    return sensors[sensor_id].channels[channel_index];
}