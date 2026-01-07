/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_SENSORS_H
#define OCRE_SENSORS_H

#include <wasm_export.h>
#include <zephyr/drivers/sensor.h>

#define CONFIG_MAX_SENSOR_NAME_LENGTH 125

typedef int32_t ocre_sensor_handle_t;

/**
 * @brief Enum representing different sensor channels
 */
typedef enum {
    SENSOR_CHANNEL_ACCELERATION,
    SENSOR_CHANNEL_GYRO,
    SENSOR_CHANNEL_MAGNETIC_FIELD,
    SENSOR_CHANNEL_LIGHT,
    SENSOR_CHANNEL_PRESSURE,
    SENSOR_CHANNEL_PROXIMITY,
    SENSOR_CHANNEL_HUMIDITY,
    SENSOR_CHANNEL_TEMPERATURE,
    // Add more channels as needed
} sensor_channel_t;

/**
 * @brief Structure representing a sensor instance
 */
typedef struct ocre_sensor_t {
    ocre_sensor_handle_t handle;
    char sensor_name[CONFIG_MAX_SENSOR_NAME_LENGTH];
    int num_channels;
    enum sensor_channel channels[CONFIG_MAX_CHANNELS_PER_SENSOR];
} ocre_sensor_t;
/**
 * @brief Initialize the sensor system
 *
 * @param exec_env WASM execution environment
 * @return 0 on success
 */
int ocre_sensors_init(wasm_exec_env_t exec_env);
/**
 * @brief Discover available sensors
 *
 * @param exec_env WASM execution environment
 * @return Number of discovered sensors, negative error code on failure
 */
int ocre_sensors_discover(wasm_exec_env_t exec_env);
/**
 * @brief Open a sensor for use
 *
 * @param exec_env WASM execution environment
 * @param handle Handle of the sensor to open
 * @return 0 on success, negative error code on failure
 */
int ocre_sensors_open(wasm_exec_env_t exec_env, ocre_sensor_handle_t handle);
/**
 * @brief Get the handle of a sensor
 *
 * @param exec_env WASM execution environment
 * @param sensor_id ID of the sensor
 * @return Sensor handle on success, negative error code on failure
 */
int ocre_sensors_get_handle(wasm_exec_env_t exec_env, int sensor_id);
/**
 * @brief Get the number of channels available in a sensor
 *
 * @param exec_env WASM execution environment
 * @param sensor_id ID of the sensor
 * @return Number of channels on success, negative error code on failure
 */
int ocre_sensors_get_channel_count(wasm_exec_env_t exec_env, int sensor_id);
/**
 * @brief Get the type of a specific sensor channel
 *
 * @param exec_env WASM execution environment
 * @param sensor_id ID of the sensor
 * @param channel_index Index of the channel
 * @return Channel type on success, negative error code on failure
 */
int ocre_sensors_get_channel_type(wasm_exec_env_t exec_env, int sensor_id, int channel_index);
/**
 * @brief Read data from a sensor channel
 *
 * @param exec_env WASM execution environment
 * @param sensor_id ID of the sensor
 * @param channel_type Type of the channel to read
 * @return Sensor value in integer format, negative error code on failure
 */
double ocre_sensors_read(wasm_exec_env_t exec_env, int sensor_id, int channel_type);

/* New string-based APIs */
int ocre_sensors_open_by_name(wasm_exec_env_t exec_env, const char *sensor_name);
int ocre_sensors_get_handle_by_name(wasm_exec_env_t exec_env, const char *sensor_name);
int ocre_sensors_get_channel_count_by_name(wasm_exec_env_t exec_env, const char *sensor_name);
int ocre_sensors_get_channel_type_by_name(wasm_exec_env_t exec_env, const char *sensor_name, int channel_index);
double ocre_sensors_read_by_name(wasm_exec_env_t exec_env, const char *sensor_name, int channel_type);

/* Utility functions */
int ocre_sensors_get_list(wasm_exec_env_t exec_env, char **name_list, int max_names);

#endif /* OCRE_SENSORS_H */
