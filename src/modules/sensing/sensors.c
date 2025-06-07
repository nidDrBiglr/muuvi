#include "sensors.h"
#include "utils.h"
#include "led.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(sensors);

ZBUS_SUBSCRIBER_DEFINE(sampler, CONFIG_SENSING_MESSAGE_QUEUE_SIZE);

static const struct device *const dht22 = DEVICE_DT_GET_ONE(aosong_dht);

void read_sensor_values(measurement_t *measurements)
{

	while (!device_is_ready(dht22)) {
		LOG_WRN("device %s is not ready yet, waiting...", dht22->name);
		k_sleep(K_MSEC(100));
	}

	LOG_INF("device %s ready, sampling...", dht22->name);

	int error = sensor_sample_fetch(dht22);

	if (error != 0) {
		LOG_ERR("dht sensor sample fetch failed, error: %d", error);
		return;
	}

	struct sensor_value temperature;
	struct sensor_value humidity;

	error = sensor_channel_get(dht22, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
	if (error != 0) {
		LOG_ERR("failed to get %s temperature reading, error %d", dht22->name, error);
		measurements->temperature = TEMP_INVALID_MEASUREMENT;
	} else {
		// convert the sensor value for temperature to a double
		double temp_c = sensor_value_to_double(&temperature);
		// divide the value by 0.005 according to ruuvi requirements
		measurements->temperature = (int16_t)(temp_c / 0.005);
	}
	error = sensor_channel_get(dht22, SENSOR_CHAN_HUMIDITY, &humidity);
	if (error != 0) {
		LOG_ERR("failed to get %s humidity reading, error %d", dht22->name, error);
		measurements->humidity = HUM_INVALID_MEASUREMENT;
	} else {
		// convert the sensor value for humidity to a double
		double hum_rh = sensor_value_to_double(&humidity);
		// divide the value by 0.0025 according to ruuvi requirements
		measurements->humidity = (uint16_t)(hum_rh / 0.0025);
	}
}

static void sensing_task(void)
{
	const struct zbus_channel *chan;
	measurement_t measurements;
	uint16_t sequence_number;

	while (!zbus_sub_wait(&sampler, &chan, K_FOREVER)) {
		if (&TRIGGER_CHAN == chan) {
			// turn on the green channel of the LED to indicate measurement collecting
			int err = zbus_chan_pub(&LED_PATTERN_CHAN, &PATTERN_COLLECTING_SENSOR,
						K_SECONDS(1));
			if (err) {
				LOG_ERR("zbus_chan_pub, error: %d", err);
				SEND_FATAL_ERROR();
			}
			read_sensor_values(&measurements);

			err = zbus_chan_read(&TRIGGER_CHAN, &sequence_number, K_SECONDS(1));
			if (err) {
				LOG_ERR("zbus_chan_read, error: %d", err);
				SEND_FATAL_ERROR();
				return;
			}
			// update the sequence number
			measurements.sequence_number = sequence_number;
			err = zbus_chan_pub(&PAYLOAD_CHAN, &measurements, K_SECONDS(1));
			if (err) {
				LOG_ERR("zbus_chan_pub, error: %d", err);
				SEND_FATAL_ERROR();
			}
		}
	}
}

K_THREAD_DEFINE(sensing_task_id, CONFIG_SENSING_THREAD_STACK_SIZE, sensing_task, NULL, NULL, NULL,
		3, 0, 0);