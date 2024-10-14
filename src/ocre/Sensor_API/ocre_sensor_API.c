// ocre_sensor_api.c

#include "ocre_sensor_api.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>

// Optional: Include logging for debugging
#if OCRE_SENSOR_API_DEBUG_ON
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ocre_sensor_api, LOG_LEVEL_DBG);
#endif

#define MAX_SUBSCRIPTIONS_PER_SENSOR 10

// Forward declarations
static void sensor_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger);

typedef struct
{
    ocre_sensor_trigger_cb callback;
    void *ptr;
    int subscription_id;
    enum sensor_trigger_type trigger_type;
} subscription_t;

typedef struct
{
    ocre_sensor_handle_t handle;
    subscription_t subscriptions[MAX_SUBSCRIPTIONS_PER_SENSOR];
    int subscription_count;
} sensor_subscription_t;

// Global array to keep track of sensor subscriptions
static sensor_subscription_t sensor_subscriptions[MAX_SENSORS];

ocre_sensor_api_status_t ocre_sensor_api_init(ocre_sensor_api_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    memset(ctx, 0, sizeof(ocre_sensor_api_ctx_t));

#if OCRE_SENSOR_API_DEBUG_ON
    LOG_DBG("Initializing Ocre Sensor API");
#endif

    // Add the init here

    return SENSOR_API_STATUS_INITIALIZED;
}

ocre_sensor_api_status_t ocre_sensor_api_discover_sensors(ocre_sensor_api_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    ctx->sensor_count = 0;

    for (int i = 0; i < MAX_SENSORS; i++)
    {
        // Typically, sensors are defined in the device tree and can be iterated via device_get_next_binding
        const struct device *dev = device_get_binding(CONFIG_SENSOR_NAME[i]);
        if (dev && device_is_ready(dev))
        {
            ocre_sensor_t *sensor = &ctx->sensors[ctx->sensor_count];
            sensor->handle.id = ctx->sensor_count;
            sensor->dev = (struct device *)dev;
            sensor->num_channels = sensor_channel_get_supported_channels(dev, sensor->channels, MAX_SENSOR_CHANNELS);
            sensor->container = NULL; // Initialize as needed

#if OCRE_SENSOR_API_DEBUG_ON
            LOG_DBG("Discovered sensor %s with %d channels", dev->name, sensor->num_channels);
#endif

            ctx->sensor_count++;
            if (ctx->sensor_count >= MAX_SENSORS)
            {
                break;
            }
        }
    }

    if (ctx->sensor_count == 0)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    return SENSOR_API_STATUS_INITIALIZED;
}

ocre_sensor_api_status_t ocre_sensor_api_open_channel(ocre_sensor_api_ctx_t *ctx, ocre_sensor_handle_t *sensor_handle)
{
    if (ctx == NULL || sensor_handle == NULL)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    // Validate sensor_handle ID
    if (sensor_handle->id < 0 || sensor_handle->id >= ctx->sensor_count)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    ocre_sensor_t *sensor = &ctx->sensors[sensor_handle->id];

#if OCRE_SENSOR_API_DEBUG_ON
    LOG_DBG("Opening channel for sensor ID %d (%s)", sensor->handle.id, sensor->dev->name);
#endif

    // Initialize the sensor if needed
    if (sensor_sample_fetch(sensor->dev) < 0)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    return SENSOR_API_STATUS_INITIALIZED;
}

ocre_sensor_api_status_t ocre_sensor_api_subscribe(
    ocre_sensor_api_ctx_t *ctx,
    ocre_sensor_handle_t sensor_handle,
    sensor_channel_t channel,
    enum sensor_trigger_type trigger_type,
    ocre_sensor_trigger_cb callback,
    int *subscription_id)
{
    if (ctx == NULL || callback == NULL || subscription_id == NULL)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    // Validate sensor_handle ID
    if (sensor_handle.id < 0 || sensor_handle.id >= ctx->sensor_count)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    ocre_sensor_t *sensor = &ctx->sensors[sensor_handle.id];
    const struct device *dev = sensor->dev;

    // Find the subscription slot
    sensor_subscription_t *sub = NULL;
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if (sensor_subscriptions[i].handle.id == sensor_handle.id)
        {
            sub = &sensor_subscriptions[i];
            break;
        }
    }

    if (sub == NULL)
    {
        // Find an empty slot
        for (int i = 0; i < MAX_SENSORS; i++)
        {
            if (sensor_subscriptions[i].handle.id == -1)
            { // Assuming -1 indicates unused
                sensor_subscriptions[i].handle.id = sensor_handle.id;
                sub = &sensor_subscriptions[i];
                break;
            }
        }
    }

    if (sub == NULL)
    {
        return SENSOR_API_STATUS_ERROR; // No available subscription slots
    }

    if (sub->subscription_count >= MAX_SUBSCRIPTIONS_PER_SENSOR)
    {
        return SENSOR_API_STATUS_ERROR; // Exceeded max subscriptions
    }

    // Assign a unique subscription ID
    int new_subscription_id = sub->subscription_count;

    // Set up the trigger
    struct sensor_trigger trigger = {
        .type = trigger_type,
        .chan = channel,
    };

    // Assign the callback
    sub->subscriptions[sub->subscription_count].callback = callback;
    sub->subscriptions[sub->subscription_count].ptr = NULL; // User can pass data via 'ptr' if needed
    sub->subscriptions[sub->subscription_count].subscription_id = new_subscription_id;
    sub->subscriptions[sub->subscription_count].trigger_type = trigger_type;

    if (sensor_trigger_set(dev, &trigger, sensor_trigger_handler) < 0)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    *subscription_id = new_subscription_id;
    sub->subscription_count++;

#if OCRE_SENSOR_API_DEBUG_ON
    LOG_DBG("Subscribed to sensor ID %d channel %d with subscription ID %d", sensor_handle.id, channel, new_subscription_id);
#endif

    return SENSOR_API_STATUS_INITIALIZED;
}

ocre_sensor_api_status_t ocre_sensor_api_subscribe(
    ocre_sensor_api_ctx_t *ctx,
    ocre_sensor_handle_t sensor_handle,
    sensor_channel_t channel,
    enum sensor_trigger_type trigger_type,
    ocre_sensor_trigger_cb callback,
    int *subscription_id)
{
    if (ctx == NULL || callback == NULL || subscription_id == NULL)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    // Validate sensor_handle ID
    if (sensor_handle.id < 0 || sensor_handle.id >= ctx->sensor_count)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    ocre_sensor_t *sensor = &ctx->sensors[sensor_handle.id];
    const struct device *dev = sensor->dev;

    // Find the subscription slot
    sensor_subscription_t *sub = NULL;
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if (sensor_subscriptions[i].handle.id == sensor_handle.id)
        {
            sub = &sensor_subscriptions[i];
            break;
        }
    }

    if (sub == NULL)
    {
        // Find an empty slot
        for (int i = 0; i < MAX_SENSORS; i++)
        {
            if (sensor_subscriptions[i].handle.id == -1)
            { // Assuming -1 indicates unused
                sensor_subscriptions[i].handle.id = sensor_handle.id;
                sub = &sensor_subscriptions[i];
                break;
            }
        }
    }

    if (sub == NULL)
    {
        return SENSOR_API_STATUS_ERROR; // No available subscription slots
    }

    if (sub->subscription_count >= MAX_SUBSCRIPTIONS_PER_SENSOR)
    {
        return SENSOR_API_STATUS_ERROR; // Exceeded max subscriptions
    }

    // Assign a unique subscription ID
    int new_subscription_id = sub->subscription_count;

    // Set up the trigger
    struct sensor_trigger trigger = {
        .type = trigger_type,
        .chan = channel,
    };

    // Assign the callback
    sub->subscriptions[sub->subscription_count].callback = callback;
    sub->subscriptions[sub->subscription_count].ptr = NULL; // User can pass data via 'ptr' if needed
    sub->subscriptions[sub->subscription_count].subscription_id = new_subscription_id;
    sub->subscriptions[sub->subscription_count].trigger_type = trigger_type;

    if (sensor_trigger_set(dev, &trigger, sensor_trigger_handler) < 0)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    *subscription_id = new_subscription_id;
    sub->subscription_count++;

#if OCRE_SENSOR_API_DEBUG_ON
    LOG_DBG("Subscribed to sensor ID %d channel %d with subscription ID %d", sensor_handle.id, channel, new_subscription_id);
#endif

    return SENSOR_API_STATUS_INITIALIZED;
}

ocre_sensor_api_status_t ocre_sensor_api_subscribe(
    ocre_sensor_api_ctx_t *ctx,
    ocre_sensor_handle_t sensor_handle,
    sensor_channel_t channel,
    enum sensor_trigger_type trigger_type,
    ocre_sensor_trigger_cb callback,
    int *subscription_id)
{
    if (ctx == NULL || callback == NULL || subscription_id == NULL)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    // Validate sensor_handle ID
    if (sensor_handle.id < 0 || sensor_handle.id >= ctx->sensor_count)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    ocre_sensor_t *sensor = &ctx->sensors[sensor_handle.id];
    const struct device *dev = sensor->dev;

    // Find the subscription slot
    sensor_subscription_t *sub = NULL;
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if (sensor_subscriptions[i].handle.id == sensor_handle.id)
        {
            sub = &sensor_subscriptions[i];
            break;
        }
    }

    if (sub == NULL)
    {
        // Find an empty slot
        for (int i = 0; i < MAX_SENSORS; i++)
        {
            if (sensor_subscriptions[i].handle.id == -1)
            { // Assuming -1 indicates unused
                sensor_subscriptions[i].handle.id = sensor_handle.id;
                sub = &sensor_subscriptions[i];
                break;
            }
        }
    }

    if (sub == NULL)
    {
        return SENSOR_API_STATUS_ERROR; // No available subscription slots
    }

    if (sub->subscription_count >= MAX_SUBSCRIPTIONS_PER_SENSOR)
    {
        return SENSOR_API_STATUS_ERROR; // Exceeded max subscriptions
    }

    // Assign a unique subscription ID
    int new_subscription_id = sub->subscription_count;

    // Set up the trigger
    struct sensor_trigger trigger = {
        .type = trigger_type,
        .chan = channel,
    };

    // Assign the callback
    sub->subscriptions[sub->subscription_count].callback = callback;
    sub->subscriptions[sub->subscription_count].ptr = NULL; // User can pass data via 'ptr' if needed
    sub->subscriptions[sub->subscription_count].subscription_id = new_subscription_id;
    sub->subscriptions[sub->subscription_count].trigger_type = trigger_type;

    if (sensor_trigger_set(dev, &trigger, sensor_trigger_handler) < 0)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    *subscription_id = new_subscription_id;
    sub->subscription_count++;

#if OCRE_SENSOR_API_DEBUG_ON
    LOG_DBG("Subscribed to sensor ID %d channel %d with subscription ID %d", sensor_handle.id, channel, new_subscription_id);
#endif

    return SENSOR_API_STATUS_INITIALIZED;
}

static void sensor_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
    // Find the sensor handle based on the device
    int sensor_id = -1;
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if (sensor_subscriptions[i].handle.id != -1 &&
            sensor_subscriptions[i].handle.id < MAX_SENSORS &&
            sensor_subscriptions[i].handle.id == i && // Adjust based on actual mapping
            device_get_binding(CONFIG_SENSOR_NAME[i]) == dev)
        {
            sensor_id = sensor_subscriptions[i].handle.id;
            break;
        }
    }

    if (sensor_id == -1)
    {
#if OCRE_SENSOR_API_DEBUG_ON
        LOG_DBG("Received trigger from unknown sensor device %s", dev->name);
#endif
        return;
    }

    sensor_subscription_t *sub = NULL;
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if (sensor_subscriptions[i].handle.id == sensor_id)
        {
            sub = &sensor_subscriptions[i];
            break;
        }
    }

    if (sub == NULL)
    {
        return;
    }

    // Iterate through subscriptions and invoke callbacks
    for (int i = 0; i < sub->subscription_count; i++)
    {
        if (sub->subscriptions[i].trigger_type == trigger->type &&
            sub->subscriptions[i].trigger_type == trigger->chan)
        { // Adjust condition as needed
            if (sub->subscriptions[i].callback)
            {
                struct sensor_value data;
                sensor_sample_fetch(dev);
                sensor_channel_get(dev, trigger->chan, &data);
                sub->subscriptions[i].callback(sub->handle, trigger->chan, &data, sub->subscriptions[i].ptr);
            }
        }
    }
}

ocre_sensor_api_status_t ocre_sensor_api_read_data(
    ocre_sensor_api_ctx_t *ctx,
    ocre_sensor_handle_t sensor_handle,
    sensor_channel_t channel,
    struct sensor_value *data)
{
    if (ctx == NULL || data == NULL)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    // Validate sensor_handle ID
    if (sensor_handle.id < 0 || sensor_handle.id >= ctx->sensor_count)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    ocre_sensor_t *sensor = &ctx->sensors[sensor_handle.id];
    const struct device *dev = sensor->dev;

#if OCRE_SENSOR_API_DEBUG_ON
    LOG_DBG("Reading data from sensor ID %d channel %d", sensor_handle.id, channel);
#endif

    if (sensor_sample_fetch(dev) < 0)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    if (sensor_channel_get(dev, channel, data) < 0)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    return SENSOR_API_STATUS_INITIALIZED;
}

ocre_sensor_api_status_t ocre_sensor_api_unsubscribe(
    ocre_sensor_api_ctx_t *ctx,
    ocre_sensor_handle_t sensor_handle,
    sensor_channel_t channel,
    int subscription_id)
{
    if (ctx == NULL)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    // Validate sensor_handle ID
    if (sensor_handle.id < 0 || sensor_handle.id >= ctx->sensor_count)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    sensor_subscription_t *sub = NULL;
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if (sensor_subscriptions[i].handle.id == sensor_handle.id)
        {
            sub = &sensor_subscriptions[i];
            break;
        }
    }

    if (sub == NULL || subscription_id < 0 || subscription_id >= sub->subscription_count)
    {
        return SENSOR_API_STATUS_ERROR;
    }

    // Remove the subscription
    for (int i = subscription_id; i < sub->subscription_count - 1; i++)
    {
        sub->subscriptions[i] = sub->subscriptions[i + 1];
    }
    sub->subscription_count--;

    // If no more subscriptions, unset the trigger
    if (sub->subscription_count == 0)
    {
        ocre_sensor_t *sensor = &ctx->sensors[sensor_handle.id];
        struct sensor_trigger trigger = {
            .type = SENSOR_TRIG_DATA_READY,
            .chan = SENSOR_CHAN_ALL,
        };
        sensor_trigger_set(sensor->dev, &trigger, NULL); // Unset trigger
        sensor_subscriptions[i].handle.id = -1;          // Mark as unused
    }

#if OCRE_SENSOR_API_DEBUG_ON
    LOG_DBG("Unsubscribed from sensor ID %d subscription ID %d", sensor_handle.id, subscription_id);
#endif

    return SENSOR_API_STATUS_INITIALIZED;
}

ocre_sensor_api_status_t ocre_sensor_api_cleanup(ocre_sensor_api_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return SENSOR_API_STATUS_ERROR;
    }

#if OCRE_SENSOR_API_DEBUG_ON
    LOG_DBG("Cleaning up Ocre Sensor API");
#endif

    // Unsubscribe all sensors
    for (int i = 0; i < ctx->sensor_count; i++)
    {
        sensor_subscription_t *sub = &sensor_subscriptions[i];
        if (sub->handle.id != -1)
        {
            ocre_sensor_t *sensor = &ctx->sensors[sub->handle.id];
            struct sensor_trigger trigger = {
                .type = SENSOR_TRIG_DATA_READY,
                .chan = SENSOR_CHAN_ALL,
            };
            sensor_trigger_set(sensor->dev, &trigger, NULL); // Unset all triggers
            sub->handle.id = -1;
            sub->subscription_count = 0;
        }
    }

    ctx->sensor_count = 0;

    // Additional cleanup steps can be added here

    return SENSOR_API_STATUS_INITIALIZED;
}

#define SENSOR_CHAN_FIRST 0
#define SENSOR_CHAN_LAST 51

static int sensor_channel_get_supported_channels(const struct device *dev, sensor_channel_t *channels, int max_channels)
{
    if (dev == NULL || channels == NULL)
    {
        return 0;
    }

    int count = 0;
    for (int i = SENSOR_CHAN_FIRST; i <= SENSOR_CHAN_LAST; i++)
    {
        struct sensor_value value;
        if (sensor_channel_get(dev, i, &value) == 0)
        {
            channels[count++] = (sensor_channel_t)i;
            if (count >= max_channels)
            {
                break;
            }
        }
    }

    return count;
}
