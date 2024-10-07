#include "ocre_sensor_runtime.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ocre_sensor_runtime, OCRE_SENSOR_RUNTIME_DEBUG_ON ? LOG_LEVEL_DBG : LOG_LEVEL_INF);

/**
 * @brief Initializes the sensor runtime environment.
 */
ocre_sensor_runtime_status_t ocre_sensor_runtime_init(ocre_sensor_runtime_ctx_t *ctx)
{
    if (!ctx)
    {
        LOG_ERR("Sensor runtime context is NULL");
        return SENSOR_RUNTIME_STATUS_ERROR;
    }

    memset(ctx, 0, sizeof(ocre_sensor_runtime_ctx_t));

    LOG_INF("Sensor runtime initialized");
    return SENSOR_RUNTIME_STATUS_INITIALIZED;
}

/**
 * @brief Discovers available sensors.
 */
int ocre_sensor_runtime_discover_sensors(ocre_sensor_runtime_ctx_t *ctx)
{
    if (!ctx)
    {
        LOG_ERR("Sensor runtime context is NULL");
        return -EINVAL;
    }

    ctx->sensor_count = 0;

    for (int i = 0; i < MAX_SENSORS; i++)
    {
        // Here, we assume that sensors are named sensor0, sensor1, etc.
        char sensor_name[12];
        snprintf(sensor_name, sizeof(sensor_name), "sensor%d", i);
        struct device *dev = device_get_binding(sensor_name);
        if (!dev)
        {
            continue; // No more sensors found
        }

        ctx->sensors[i].dev = dev;
        ctx->sensors[i].container = NULL; // Associate with container as needed

        // Discover supported channels
        ctx->sensors[i].num_channels = 0;
        for (int ch = 0; ch < MAX_SENSOR_CHANNELS; ch++)
        {
            // Example: Check if the sensor supports a particular channel
            // This part may vary based on how channels are defined in your system
            // Here, we assume all channels are supported for demonstration
            ctx->sensors[i].channels[ch] = ch; // Assign channel enum
            ctx->sensors[i].num_channels++;
        }

        LOG_INF("Discovered sensor: %s with %d channels", sensor_name, ctx->sensors[i].num_channels);
        ctx->sensor_count++;

        if (ctx->sensor_count >= MAX_SENSORS)
        {
            break;
        }
    }

    LOG_INF("Total sensors discovered: %d", ctx->sensor_count);
    return ctx->sensor_count;
}

/**
 * @brief Opens a sensor channel.
 */
int ocre_sensor_runtime_open_channel(ocre_sensor_runtime_ctx_t *ctx, int sensor_id, sensor_channel_t channel)
{
    if (!ctx || sensor_id < 0 || sensor_id >= ctx->sensor_count)
    {
        LOG_ERR("Invalid sensor ID");
        return -EINVAL;
    }

    ocre_sensor_t *sensor = &ctx->sensors[sensor_id];
    if (!sensor->dev)
    {
        LOG_ERR("Sensor device not initialized");
        return -ENODEV;
    }

    // Example: Configure the sensor channel as needed
    // This will depend on the specific sensor and channel
    // For demonstration, we simply log the action
    LOG_INF("Opening channel %d for sensor ID %d", channel, sensor_id);

    // In a real implementation, you might configure sensor attributes here

    return 0;
}

/**
 * @brief Trigger callback wrapper
 */
static void sensor_trigger_callback(const struct device *dev, const struct sensor_trigger *trigger)
{
    // Retrieve the sensor ID and channel from device or trigger if possible
    // This requires mapping devices to sensor IDs and channels
    // For demonstration, we'll assume some mapping exists
    // You need to implement this part based on your system

    int sensor_id = 0;                                      // Example sensor ID
    sensor_channel_t channel = SENSOR_CHANNEL_ACCELERATION; // Example channel

    struct sensor_value data;
    sensor_channel_get(dev, channel, &data);

    // Call the user-defined callback
    // You need to store and retrieve the user callback based on sensor_id and channel
    // This example does not implement callback storage

    // Example:
    // ocre_sensor_trigger_cb user_cb = get_user_callback(sensor_id, channel);
    // if (user_cb) {
    //     user_cb(sensor_id, channel, &data);
    // }
}

/**
 * @brief Sets a trigger for a sensor channel.
 */
int ocre_sensor_runtime_set_trigger(ocre_sensor_runtime_ctx_t *ctx, int sensor_id, sensor_channel_t channel, enum sensor_trigger_type trigger_type, ocre_sensor_trigger_cb callback)
{
    if (!ctx || sensor_id < 0 || sensor_id >= ctx->sensor_count)
    {
        LOG_ERR("Invalid sensor ID");
        return -EINVAL;
    }

    ocre_sensor_t *sensor = &ctx->sensors[sensor_id];
    if (!sensor->dev)
    {
        LOG_ERR("Sensor device not initialized");
        return -ENODEV;
    }

    struct sensor_trigger trigger = {
        .type = trigger_type,
        .chan = channel,
    };

    // Store the callback associated with sensor_id and channel
    // You need to implement a mechanism to store and retrieve callbacks

    // Example:
    // store_user_callback(sensor_id, channel, callback);

    int ret = sensor_trigger_set(sensor->dev, &trigger, sensor_trigger_callback);
    if (ret < 0)
    {
        LOG_ERR("Failed to set trigger for sensor ID %d, channel %d", sensor_id, channel);
        return ret;
    }

    LOG_INF("Trigger set for sensor ID %d, channel %d", sensor_id, channel);
    return 0;
}

/**
 * @brief Reads data from a sensor channel.
 */
int ocre_sensor_runtime_read_data(ocre_sensor_runtime_ctx_t *ctx, int sensor_id, sensor_channel_t channel, struct sensor_value *data)
{
    if (!ctx || !data || sensor_id < 0 || sensor_id >= ctx->sensor_count)
    {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }

    ocre_sensor_t *sensor = &ctx->sensors[sensor_id];
    if (!sensor->dev)
    {
        LOG_ERR("Sensor device not initialized");
        return -ENODEV;
    }

    int ret = sensor_channel_get(sensor->dev, channel, data);
    if (ret < 0)
    {
        LOG_ERR("Failed to get data from sensor ID %d, channel %d", sensor_id, channel);
        return ret;
    }

    LOG_INF("Sensor ID %d, Channel %d data: %d.%06d", sensor_id, channel, data->val1, data->val2);
    return 0;
}

/**
 * @brief Closes a sensor channel.
 */
int ocre_sensor_runtime_close_channel(ocre_sensor_runtime_ctx_t *ctx, int sensor_id, sensor_channel_t channel)
{
    if (!ctx || sensor_id < 0 || sensor_id >= ctx->sensor_count)
    {
        LOG_ERR("Invalid sensor ID");
        return -EINVAL;
    }

    ocre_sensor_t *sensor = &ctx->sensors[sensor_id];
    if (!sensor->dev)
    {
        LOG_ERR("Sensor device not initialized");
        return -ENODEV;
    }

    // Example: Disable trigger if set
    // You need to implement the logic based on how triggers are managed
    // For demonstration, we simply log the action
    LOG_INF("Closing channel %d for sensor ID %d", channel, sensor_id);

    // In a real implementation, you might disable triggers or perform other cleanup

    return 0;
}

/**
 * @brief Cleans up the sensor runtime environment.
 */
int ocre_sensor_runtime_cleanup(ocre_sensor_runtime_ctx_t *ctx)
{
    if (!ctx)
    {
        LOG_ERR("Sensor runtime context is NULL");
        return -EINVAL;
    }

    // Example: Disable all triggers and close all channels
    for (int i = 0; i < ctx->sensor_count; i++)
    {
        ocre_sensor_t *sensor = &ctx->sensors[i];
        if (sensor->dev)
        {
            // Disable triggers as needed
            // Example: sensor_trigger_set(sensor->dev, NULL, NULL);
            LOG_INF("Cleaning up sensor ID %d", i);
        }
    }

    memset(ctx, 0, sizeof(ocre_sensor_runtime_ctx_t));
    LOG_INF("Sensor runtime cleaned up");
    return 0;
}
