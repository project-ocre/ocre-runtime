1. Initializing the Sensor Environment

```
ocre_sensors_status_t status;

status = ocre_sensors_init();

if (status != SENSOR_API_STATUS_INITIALIZED) {

// Handle initialization error

}
```

2. Discovering Sensors

```
ocre_sensor_t sensors[MAX_SENSORS];

status = ocre_sensors_discover_sensors(sensors);

if (status != SENSOR_API_STATUS_INITIALIZED) {

// Handle discovery error

}
```

3. Opening a Sensor Channel

```
ocre_sensor_handle_t sensor_handle;

status = ocre_sensors_open_channel(&sensor_handle);

if (status != 0) {

// Handle channel open error

}
```

4. Reading Sensor Data

```
ocre_sensors_sample_t sample = sensor_read_sample(&sensor_handle);
```

  

5. Accessing Specific Sensor Channels

```
sensor_channel_t temperature = sensor_get_channel(sample, SENSOR_CHANNEL_TEMPERATURE);
```

6. Setting a Sensor Trigger

```
int subscription_id;

status = ocre_sensors_set_trigger(sensor_handle, SENSOR_CHANNEL_TEMPERATURE, DATA_READY, my_callback, &subscription_id);

if (status != 0) {

// Handle trigger set error

}
```

7. Clearing a Sensor Trigger

```
status = ocre_sensors_clear_trigger(sensor_handle, SENSOR_CHANNEL_TEMPERATURE, subscription_id);

if (status != 0) {

// Handle trigger clear error

}```

8. Cleaning Up the Sensor Environment

```status = ocre_sensors_cleanup();

if (status != 0) {

// Handle cleanup error

}
```

Usage

```

#include <stdio.h>

#include <zephyr/kernel.h>

#include "ocre_sensors.h"

  

// Define the callback function to handle trigger events

void my_callback(ocre_sensor_handle_t sensor_handle, sensor_channel_t channel, const ocre_sensor_value *data, void *ptr) {

printf("Trigger event on sensor %d, channel %d\n", sensor_handle.id, channel);

printf("Sensor data: Integer = %d, Floating = %d\n", data->integer, data->floating);

}

  

// Main function demonstrating the use of the Ocre Sensors API

void main(void) {

	ocre_sensors_status_t status;

	ocre_sensor_t sensors[MAX_SENSORS]; // Array to hold discovered sensors

	ocre_sensor_handle_t sensor_handle; // Handle for the sensor to be used

	int subscription_id; // ID for the trigger subscription

  

// Step 1: Initialize the sensors environment

	status = ocre_sensors_init();

	if (status != SENSOR_API_STATUS_INITIALIZED) {

		printf("Failed to initialize sensors. Status: %d\n", status);

		return;

	}

  

// Step 2: Discover available sensors

	status = ocre_sensors_discover_sensors(sensors);

	if (status != SENSOR_API_STATUS_INITIALIZED) {

		printf("Failed to discover sensors. Status: %d\n", status);

		return;

	}

  

// Step 3: Open a channel for the first discovered sensor

	sensor_handle = sensors[0].handle; // Assuming at least one sensor is available

	status = ocre_sensors_open_channel(&sensor_handle);

	if (status != 0) {

		printf("Failed to open sensor channel. Status: %d\n", status);

		return;

	}

  

// Step 4: Read a sample from the sensor

	ocre_sensors_sample_t sample = sensor_read_sample(&sensor_handle);

	sensor_channel_t temperature = sensor_get_channel(sample, SENSOR_CHANNEL_TEMPERATURE);

	printf("Current temperature: %d\n", temperature);

  

// Step 5: Set a trigger on the temperature channel

status = ocre_sensors_set_trigger(sensor_handle, SENSOR_CHANNEL_TEMPERATURE, DATA_READY, my_callback, &subscription_id);

	if (status != 0) {

		printf("Failed to set trigger. Status: %d\n", status);

		return;

	}

  

// Simulate waiting for trigger events (replace with actual logic as needed)

	k_sleep(K_SECONDS(10)); // Wait for 10 seconds to allow triggers to occur

  

// Step 6: Clear the trigger subscription

	status = ocre_sensors_clear_trigger(sensor_handle, SENSOR_CHANNEL_TEMPERATURE, subscription_id);

		if (status != 0) {

		printf("Failed to clear trigger. Status: %d\n", status);

		return;

	}

  

// Step 7: Clean up the sensors environment

	status = ocre_sensors_cleanup();

		if (status != 0) {

		printf("Failed to clean up sensors. Status: %d\n", status);

		return;

	}

  

	printf("Sensor operations completed successfully.\n");

}
```