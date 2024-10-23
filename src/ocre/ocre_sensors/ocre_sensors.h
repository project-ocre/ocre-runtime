/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_SENSORS_H
#define OCRE_SENSORS_H

// #include <zephyr/sensor.h>
#include <zephyr/kernel.h>

#include "../ocre_container_runtime/ocre_container_runtime.h"

/* Debug flag for sensor API (0: OFF, 1: ON) */
#define OCRE_SENSOR_API_DEBUG_ON 1

/* Static maximum number of sensors supported (used when debugging is enabled) */
#if OCRE_SENSOR_API_DEBUG_ON
// TO DO: max sensor we should set after we discover sensors and get to know how many we have in Device tree
#define MAX_SENSORS 20
#define MAX_SENSOR_CHANNELS 5
#endif

/**
 * @brief Handle type for sensors.
 */
typedef struct ocre_sensor_handle_t
{
    int id;
    const char *device_name;
    struct device sensor_device;
} ocre_sensor_handle_t;

/** TO DO update this considering all posible and needed status
 * @brief Enum representing the possible status of the sensor API
 */
typedef enum
{
    SENSOR_API_STATUS_UNKNOWN,        ///< Status is unknown.
    SENSOR_API_STATUS_INITIALIZED,    ///< API has been initialized.
    SENSOR_API_STATUS_CHANNEL_OPENED, ///< Channel has been opened .
    SENSOR_API_STATUS_OKEY,
    SENSOR_API_STATUS_UNINITIALIZED,
    SENSOR_API_STATUS_ERROR ///< An error occurred with the sensor API.
} ocre_sensors_status_t;

/**
 * @brief Enum representing different sensor channels
 */
typedef enum
{
    SENSOR_CHANNEL_ACCELERATION = 0x0C,
    SENSOR_CHANNEL_GYRO = 0x50,
    SENSOR_CHANNEL_MAGNETIC_FIELD = 0x32,
    SENSOR_CHANNEL_LIGHT = 0x0F,
    SENSOR_CHANNEL_PRESSURE = 0x7A,
    SENSOR_CHANNEL_PROXIMITY = 0x19,
    SENSOR_CHANNEL_HUMIDITY = 0x78,
    SENSOR_CHANNEL_TEMPERATURE = 0xCC,
    SENSOR_CHANNEL_RNG = 0x666,
    // Add more channels as needed
} sensor_channel_t;

/**
 * @brief Structure representing a sensor instance
 */
typedef struct ocre_sensor_t
{
    ocre_sensor_handle_t handle; ///< Sensor handle
    int num_channels;            ///< Number of supported channels + TO DO Set it when we discover this
    sensor_channel_t channels[]; ///< Supported channels
} ocre_sensor_t;

/**
 * @brief Structure representing a sensor value (integer and fractional components).
 */
typedef struct ocre_sensor_value_t
{
    int32_t integer;
    int32_t floating;
} ocre_sensor_value_t;

/**
 * @brief Structure representing a sensor sample
 */
typedef struct ocre_sensors_sample_t
{
    ocre_sensor_value_t value;
} ocre_sensors_sample_t;

/**
 * @brief Callback type for sensor triggers
 *
 * @param sensor_handle Handle of the sensor triggering the event
 * @param channel Channel that triggered the event
 * @param data Pointer to the sensor data
 * @param ptr User-defined pointer
 */
typedef void (*ocre_sensor_trigger_cb)(ocre_sensor_handle_t sensor_handle, sensor_channel_t channel, const struct ocre_sensor_value *data, void *ptr);

/**
 * @brief Initializes the ocre sensors environment.
 *
 * This function initializes the sensors subsystem
 *
 * @return Current status of the sensors environment.
 */
ocre_sensors_status_t ocre_sensors_init();

/**
 * @brief Discovers all available sensors.
 *
 * This function enumerates all available sensors and populates the sensor API context.
 *
 * @param sensors Pointer to the sensor list to set the discovered info about available sensors
 * @param dev Pointer to a device of interest will be used as the set of devices to visit.
 * @param sensors_count counter to save all number of all found sensors
 * @return Status of sensors envroinment
 */
ocre_sensors_status_t ocre_sensors_discover_sensors(ocre_sensor_t *sensors, int *sensors_count);
/**
 * @brief Opens a sensor channel.
 *
 * This function opens a communication channel with the specified sensor,
 * allowing the system to begin interacting with the sensor to obtain data.
 * Typically, this involves configuring hardware or setting up the necessary
 * communication protocols to start receiving data from the sensor.
 *
 * @param sensor_handle Pointer to the sensor handle that identifies the sensor to be
 * opened.
 * @return 0 on success, or a negative value on error.
 *         Returns an error code if the sensor channel could not be opened.
 */
ocre_sensors_status_t ocre_sensors_open_channel(ocre_sensor_handle_t *sensor_handle);

/**
 * @brief Reads a sample from the specified sensor.
 *
 * This function retrieves the most recent data sample from the sensor identified
 * by the provided sensor handle. The returned sample contains the raw sensor data,
 * which can later be processed or interpreted based on the sensor type (e.g., temperature, acceleration).
 *
 * @param sensor_handle Pointer to the sensor handle from which to read a sample.
 * @return ocre_sensors_sample_t A structure containing the sensor data sample.
 *         This may include values such as temperature, acceleration, or other metrics depending on the sensor.
 */
ocre_sensors_sample_t sensor_read_sample(ocre_sensor_handle_t *sensor_handle);

/**
 * @brief Retrieves data from a specific sensor channel.
 *
 * This function extracts and returns the data for a specific channel (e.g., temperature,
 * pressure, acceleration) from the provided sensor sample. A sample may contain multiple
 * channels of data, and this function helps in isolating the desired channel's data.
 *
 * @param sample The sensor sample structure that holds the raw data from the sensor.
 * @param channel The specific channel to retrieve data from (e.g., SENSOR_CHANNEL_TEMPERATURE).
 * @return sensor_channel_t The value of the requested channel within the sample.
 *         If the channel is invalid or unavailable, the function may return an error code or placeholder value.
 */
ocre_sensor_value_t sensor_get_channel(ocre_sensors_sample_t sample, sensor_channel_t channel);

/**
 * @brief Sets a trigger for a sensor channel.
 *
 * This function configures a specific trigger(enum sensor_trigger_type trigger_type) for a specific sensor channel(sensor_channel_t channel).
 *
 * @param sensor_handle Handle of the sensor.
 * @param channel Channel to set the trigger on.
 * @param trigger_type Type of trigger (e.g., data ready, threshold).
 * @param callback Callback function to be called when the trigger occurs.
 * @param subscription_id Pointer to store the subscription ID.
 * @return Status of sensors envroinment
 */
ocre_sensors_status_t ocre_sensors_set_trigger(ocre_sensor_handle_t sensor_handle, sensor_channel_t channel, enum sensor_trigger_type trigger_type, ocre_sensor_trigger_cb callback, int *subscription_id);

/**
 * @brief Unsubscribes from a sensor trigger.
 *
 * This function removes a previously set trigger for a specific sensor channel.
 *
 * @param sensor_handle Handle of the sensor from which to remove the trigger.
 * @param channel The specific channel (e.g., SENSOR_CHANNEL_TEMPERATURE) from which the trigger should be removed.
 * @param subscription_id ID of the subscription representing the trigger to be removed.
 * @return Status of sensors envroinment
 */
ocre_sensors_status_t ocre_sensors_clear_trigger(ocre_sensor_handle_t sensor_handle, sensor_channel_t channel, int subscription_id);

/**
 * @brief Cleans up the sensors environment.
 *
 * This function performs necessary cleanup of the sensor subsystem, such as releasing resources,
 * closing any active sensor channels, and stopping any background operations related to sensors.
 * It should be called when the sensor subsystem is no longer needed, ensuring that system resources
 * are properly freed and that the sensor environment is in a consistent state.
 *
 * @param ctx Pointer to the sensor API context structure that holds the overall state and configuration of the sensors.
 * @return Status of sensors envroinment
 */
ocre_sensors_status_t ocre_sensors_cleanup();

#endif /* OCRE_SENSORS_H */
