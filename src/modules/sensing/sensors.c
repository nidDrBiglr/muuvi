#include "sensors.h"

#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "../led/led.h"
#include "../events/event_bus.h"

LOG_MODULE_REGISTER(sensors);

void read_sensor_values(measurement_t *measurements)
{
	LOG_INF("generating random measurement values...");
	// generate a random humidity value (0 to 40000, mapped to 0% to 100% in 0.0025
	// increments)
	measurements->humidity = sys_rand32_get() % 40001;
	// generate a random temperature value (-32767 to 32767, mapped to -163.835 to +163.835
	// degrees in 0.005 increments)
	measurements->temperature = (sys_rand32_get() % 65536) - 32768;
}

void sensors_init()
{
	// todo: once sensors are connected, init them accordingly
}

void sensors_run(void)
{
	app_event_t evt;
	while (true) {
		int ret = event_bus_get(&evt, K_FOREVER);
		if (ret == 0 && evt.type == SENSOR_SAMPLE_EVENT) {
			LOG_INF("Received SENSOR_SAMPLE_EVENT");
			set_led_pattern(&PATTERN_COLLECTING_SENSOR);
			measurement_t measurement;
			read_sensor_values(&measurement);

			LOG_INF("updated advertising values: temperature: %f, humidity: %f",
				measurement.temperature * 0.005, measurement.humidity * 0.0025);

			app_event_t new_evt = {.type = SENSOR_DATA_READY_EVENT,
					       .data.measurement = measurement};

			event_bus_post(&new_evt);
		}
		k_sleep(POLL_INTERVAL);
	}
}