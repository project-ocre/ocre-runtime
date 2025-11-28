/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <ocre/ocre.h>
LOG_MODULE_DECLARE(ocre_sensors, OCRE_LOG_LEVEL);

#include "ocre_sensors.h"

#define DEVICE_NODE	 DT_PATH(devices)
#define HAS_DEVICE_NODES DT_NODE_EXISTS(DEVICE_NODE) && DT_PROP_LEN(DEVICE_NODE, device_list) > 1

/* Define sensor discovery names from device tree if available */
#if (CONFIG_OCRE_SENSORS) && (HAS_DEVICE_NODES)
#define EXTRACT_LABELS(node_id, prop, idx) DT_PROP_BY_PHANDLE_IDX_OR(node_id, prop, idx, label, "undefined")
static const char *sensor_discovery_names[] = {
	DT_FOREACH_PROP_ELEM_SEP(DEVICE_NODE, device_list, EXTRACT_LABELS, (, ))};
static int sensor_names_count = sizeof(sensor_discovery_names) / sizeof(sensor_discovery_names[0]);
#else
static const char *sensor_discovery_names[] = {};
static int sensor_names_count = 0;
#endif

typedef struct {
	const struct device *device;
	ocre_sensor_t info;
	bool in_use;
} ocre_sensor_internal_t;

static ocre_sensor_internal_t sensors[CONFIG_MAX_SENSORS] = {0};
static int sensor_count = 0;

static int set_opened_channels(const struct device *dev, enum sensor_channel *channels)
{
	if (!channels) {
		LOG_ERR("Channels array is NULL");
		return -EINVAL;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("Device is not ready");
		return -ENODEV;
	}

	if (sensor_sample_fetch(dev) < 0) {
		LOG_WRN("Failed to fetch sensor data - sensor might not be initialized");
	}

	int count = 0;
	struct sensor_value value = {};

	for (int channel = 0; channel < SENSOR_CHAN_ALL && count < CONFIG_MAX_CHANNELS_PER_SENSOR; channel++) {
		if (channel != SENSOR_CHAN_ACCEL_XYZ && channel != SENSOR_CHAN_GYRO_XYZ &&
		    channel != SENSOR_CHAN_MAGN_XYZ && channel != SENSOR_CHAN_POS_DXYZ) {
			if (sensor_channel_get(dev, channel, &value) == 0) {
				channels[count] = channel;
				count++;
			}
		}
	}
	return count;
}

int ocre_sensors_init(wasm_exec_env_t exec_env)
{
	memset(sensors, 0, sizeof(sensors));
	sensor_count = 0;
	return 0;
}

int ocre_sensors_open(wasm_exec_env_t exec_env, ocre_sensor_handle_t handle)
{
	if (handle < 0 || handle >= sensor_count || !sensors[handle].in_use) {
		LOG_ERR("Invalid sensor handle: %d", handle);
		return -EINVAL;
	}

	if (!device_is_ready(sensors[handle].device)) {
		LOG_ERR("Device %s is not ready", sensors[handle].info.sensor_name);
		return -ENODEV;
	}

	return 0;
}

int ocre_sensors_discover(wasm_exec_env_t exec_env)
{
	memset(sensors, 0, sizeof(sensors));
	sensor_count = 0;

	const struct device *dev = NULL;
	size_t device_count = z_device_get_all_static(&dev);

	if (!dev) {
		LOG_ERR("Device list is NULL. Possible memory corruption!");
		return -EINVAL;
	}
	if (device_count == 0) {
		LOG_ERR("No static devices found");
		return -ENODEV;
	}

	LOG_INF("Total static devices found: %zu", device_count);

	for (size_t i = 0; i < device_count && sensor_count < CONFIG_MAX_SENSORS; i++) {
		if (!dev[i].name) {
			LOG_ERR("Device %zu has NULL name, skipping!", i);
			continue;
		}

		LOG_INF("Checking device: %s", dev[i].name);

		bool sensor_found = false;
		for (int j = 0; j < sensor_names_count; j++) {
			if (strcmp(dev[i].name, sensor_discovery_names[j]) == 0) {
				sensor_found = true;
				break;
			}
		}
		if (!sensor_found) {
			LOG_WRN("Skipping device, not in sensor list: %s", dev[i].name);
			continue;
		}

		const struct sensor_driver_api *api = (const struct sensor_driver_api *)dev[i].api;
		if (!api || !api->channel_get) {
			LOG_WRN("Device %s does not support sensor API or channel_get, skipping", dev[i].name);
			continue;
		}

		if (sensor_count >= CONFIG_MAX_SENSORS) {
			LOG_WRN("Max sensor limit reached, skipping device: %s", dev[i].name);
			continue;
		}

		ocre_sensor_internal_t *sensor = &sensors[sensor_count];
		sensor->device = &dev[i];
		sensor->in_use = true;
		sensor->info.handle = sensor_count;

		strncpy(sensor->info.sensor_name, dev[i].name, CONFIG_MAX_SENSOR_NAME_LENGTH - 1);
		sensor->info.sensor_name[CONFIG_MAX_SENSOR_NAME_LENGTH - 1] = '\0';

		sensor->info.num_channels = set_opened_channels(&dev[i], sensor->info.channels);
		if (sensor->info.num_channels <= 0) {
			LOG_WRN("Device %s does not have opened channels, skipping", dev[i].name);
			continue;
		}

		LOG_INF("Device has %d channels", sensor->info.num_channels);
		sensor_count++;
	}

	LOG_INF("Discovered %d sensors", sensor_count);
	return sensor_count;
}

int ocre_sensors_get_handle(wasm_exec_env_t exec_env, int sensor_id)
{
	if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use) {
		return -EINVAL;
	}
	return sensors[sensor_id].info.handle;
}

int ocre_sensors_get_name(wasm_exec_env_t exec_env, int sensor_id, char *buffer, int buffer_size)
{
	if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use || !buffer) {
		return -EINVAL;
	}

	int name_len = strlen(sensors[sensor_id].info.sensor_name);
	if (name_len >= buffer_size) {
		return -ENOSPC;
	}

	strncpy(buffer, sensors[sensor_id].info.sensor_name, buffer_size - 1);
	buffer[buffer_size - 1] = '\0';
	return name_len;
}

int ocre_sensors_get_channel_count(wasm_exec_env_t exec_env, int sensor_id)
{
	if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use) {
		return -EINVAL;
	}
	return sensors[sensor_id].info.num_channels;
}

int ocre_sensors_get_channel_type(wasm_exec_env_t exec_env, int sensor_id, int channel_index)
{
	if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use || channel_index < 0 ||
	    channel_index >= sensors[sensor_id].info.num_channels) {
		return -EINVAL;
	}
	return sensors[sensor_id].info.channels[channel_index];
}

double ocre_sensors_read(wasm_exec_env_t exec_env, int sensor_id, int channel_type)
{
	if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use) {
		return -EINVAL;
	}

	const struct device *dev = sensors[sensor_id].device;
	struct sensor_value value = {};

	if (sensor_sample_fetch(dev) < 0) {
		LOG_ERR("Failed to fetch sensor data");
		return -EIO;
	}

	if (sensor_channel_get(dev, channel_type, &value) < 0) {
		LOG_ERR("Failed to get scalar channel data");
		return -EIO;
	}

	return sensor_value_to_double(&value);
}

static int find_sensor_by_name(const char *name)
{
	if (!name) {
		return -EINVAL;
	}

	for (int i = 0; i < sensor_count; i++) {
		if (sensors[i].in_use && strcmp(sensors[i].info.sensor_name, name) == 0) {
			return i;
		}
	}

	return -ENOENT;
}

int ocre_sensors_open_by_name(wasm_exec_env_t exec_env, const char *sensor_name)
{
	int sensor_id = find_sensor_by_name(sensor_name);
	if (sensor_id < 0) {
		LOG_ERR("Sensor not found: %s", sensor_name);
		return -ENOENT;
	}

	return ocre_sensors_open(exec_env, sensor_id);
}

int ocre_sensors_get_handle_by_name(wasm_exec_env_t exec_env, const char *sensor_name)
{
	int sensor_id = find_sensor_by_name(sensor_name);
	if (sensor_id < 0) {
		return -ENOENT;
	}

	return sensors[sensor_id].info.handle;
}

int ocre_sensors_get_channel_count_by_name(wasm_exec_env_t exec_env, const char *sensor_name)
{
	int sensor_id = find_sensor_by_name(sensor_name);
	if (sensor_id < 0) {
		return -ENOENT;
	}

	return sensors[sensor_id].info.num_channels;
}

int ocre_sensors_get_channel_type_by_name(wasm_exec_env_t exec_env, const char *sensor_name, int channel_index)
{
	int sensor_id = find_sensor_by_name(sensor_name);
	if (sensor_id < 0) {
		return -ENOENT;
	}

	return ocre_sensors_get_channel_type(exec_env, sensor_id, channel_index);
}

double ocre_sensors_read_by_name(wasm_exec_env_t exec_env, const char *sensor_name, int channel_type)
{
	int sensor_id = find_sensor_by_name(sensor_name);
	if (sensor_id < 0) {
		LOG_ERR("Sensor not found: %s", sensor_name);
		return -ENOENT;
	}

	return ocre_sensors_read(exec_env, sensor_id, channel_type);
}

int ocre_sensors_get_list(wasm_exec_env_t exec_env, char **name_list, int max_names)
{
	if (!name_list || max_names <= 0) {
		return -EINVAL;
	}

	int count = 0;
	for (int i = 0; i < sensor_count && count < max_names; i++) {
		if (sensors[i].in_use) {
			name_list[count] = (char *)sensors[i].info.sensor_name;
			count++;
		}
	}

	return count;
}
