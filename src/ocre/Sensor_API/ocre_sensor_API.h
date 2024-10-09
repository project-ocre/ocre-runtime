/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_SENSOR_API_H
#define OCRE_SENSOR_API_H

#include <zephyr/sensor.h>
#include <zephyr/kernel.h>

#include <ocre/container_runtime.h>

/* Debug flag for sensor API (0: OFF, 1: ON) */
#define OCRE_SENSOR_API_DEBUG_ON 0

/* Maximum number of sensors supported */
#define MAX_SENSORS 20

/* Maximum number of channels per sensor */
#define MAX_SENSOR_CHANNELS 5

/**
 * @brief Enum representing the possible status of the sensor API
 */
typedef enum
{
    SENSOR_API_STATUS_UNKNOWN,     ///< Status is unknown.
    SENSOR_API_STATUS_INITIALIZED, ///< API has been initialized.
    SENSOR_API_STATUS_ERROR        ///< An error occurred with the sensor API.
} ocre_sensor_api_status_t;

/**
 * @brief Enum representing different sensor channels
 */
typedef enum
{
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
typedef struct ocre_sensor_t
{
    struct device *dev;                             ///< Zephyr sensor device
    sensor_channel_t channels[MAX_SENSOR_CHANNELS]; ///< Supported channels
    int num_channels;                               ///< Number of supported channels
    ocre_container_t *container;                    ///< Associated container
} ocre_sensor_t;

/**
 * @brief Structure representing the sensor API context
 */
typedef struct ocre_sensor_api_ctx_t
{
    ocre_sensor_t sensors[MAX_SENSORS]; ///< Array of sensor instances
    int sensor_count;                   ///< Number of sensors discovered
} ocre_sensor_api_ctx_t;

/**
 * @brief Callback type for sensor triggers
 *
 * @param sensor_id ID of the sensor triggering the event
 * @param channel Channel that triggered the event
 * @param data Pointer to the sensor data
 */
typedef void (*ocre_sensor_trigger_cb)(int sensor_id, sensor_channel_t channel, const struct sensor_value *data);

/**
 * @brief Initializes the sensor API environment.
 *
 * This function initializes the sensor subsystem within the container runtime.
 *
 * @param ctx Pointer to the sensor API context structure.
 * @return Current status of the sensor API.
 */
ocre_sensor_api_status_t ocre_sensor_api_init(ocre_sensor_api_ctx_t *ctx);

/**
 * @brief Discovers available sensors.
 *
 * This function enumerates all available sensors and populates the sensor API context.
 *
 * @param ctx Pointer to the sensor API context structure.
 * @return Number of sensors discovered, or a negative value on error.
 */
int ocre_sensor_api_discover_sensors(ocre_sensor_api_ctx_t *ctx);

/**
 * @brief Opens a sensor channel.
 *
 * This function opens a communication channel with the specified sensor and channel.
 *
 * @param ctx Pointer to the sensor API context structure.
 * @param sensor_id ID of the sensor to open.
 * @param channel Channel to open.
 * @return 0 on success, or a negative value on error.
 */
int ocre_sensor_api_open_channel(ocre_sensor_api_ctx_t *ctx, int sensor_id, sensor_channel_t channel);

/**
 * @brief Sets a trigger for a sensor channel.
 *
 * This function configures a trigger for a specific sensor channel.
 *
 * @param ctx Pointer to the sensor API context structure.
 * @param sensor_id ID of the sensor.
 * @param channel Channel to set the trigger on.
 * @param trigger_type Type of trigger (e.g., data ready, threshold).
 * @param callback Callback function to be called when the trigger occurs.
 * @return 0 on success, or a negative value on error.
 */
int ocre_sensor_api_set_trigger(ocre_sensor_api_ctx_t *ctx, int sensor_id, sensor_channel_t channel, enum sensor_trigger_type trigger_type, ocre_sensor_trigger_cb callback);

/**
 * @brief Reads data from a sensor channel.
 *
 * This function retrieves data from the specified sensor channel.
 *
 * @param ctx Pointer to the sensor API context structure.
 * @param sensor_id ID of the sensor.
 * @param channel Channel to read data from.
 * @param data Pointer to store the retrieved sensor data.
 * @return 0 on success, or a negative value on error.
 */
int ocre_sensor_api_read_data(ocre_sensor_api_ctx_t *ctx, int sensor_id, sensor_channel_t channel, struct sensor_value *data);

/**
 * @brief Closes a sensor channel.
 *
 * This function closes the specified sensor channel.
 *
 * @param ctx Pointer to the sensor API context structure.
 * @param sensor_id ID of the sensor.
 * @param channel Channel to close.
 * @return 0 on success, or a negative value on error.
 */
int ocre_sensor_api_close_channel(ocre_sensor_api_ctx_t *ctx, int sensor_id, sensor_channel_t channel);

/**
 * @brief Cleans up the sensor API environment.
 *
 * This function performs necessary cleanup of the sensor subsystem.
 *
 * @param ctx Pointer to the sensor API context structure.
 * @return 0 on success, or a negative value on error.
 */
int ocre_sensor_api_cleanup(ocre_sensor_api_ctx_t *ctx);

#endif /* OCRE_SENSOR_API_H */
