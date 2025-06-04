#include "sensors.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(sensors);

static const struct device *const dht22 = DEVICE_DT_GET_ONE(aosong_dht);

void read_sensor_values(measurement_t *measurements)
{

	int error = sensor_sample_fetch(dht22);

	if (error != 0) {
		LOG_ERR("dht sensor sample fetch failed, error: %d", error);
		return;
	}

	struct sensor_value temperature;
	struct sensor_value humidity;

	error = sensor_channel_get(dht22, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
	if (error != 0) {
		LOG_ERR("failed to get dht22 temperature reading, error %d", error);
		measurements->temperature = 0;
	}
	error = sensor_channel_get(dht22, SENSOR_CHAN_HUMIDITY, &humidity);
	if (error != 0) {
		LOG_ERR("failed to get dht22 humidity reading, error %d", error);
		measurements->humidity = 0;
	}
	// convert the sensor value for temperature to a double
	double temp_c = sensor_value_to_double(&temperature);
	// divide the value by 0.005 according to ruuvi requirements
	measurements->temperature = (int16_t)(temp_c / 0.005);
	// convert the sensor value for humidity to a double
	double hum_rh = sensor_value_to_double(&humidity);
	// divide the value by 0.0025 according to ruuvi requirements
	measurements->humidity = (uint16_t)(hum_rh / 0.0025);
}

void init_sensors()
{
	while (!device_is_ready(dht22)) {
		LOG_WRN("device %s is not ready yet, waiting...", dht22->name);
		k_sleep(K_MSEC(100));
	}

	LOG_INF("DHT22 initialized");
}