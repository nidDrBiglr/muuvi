#include "sensors.h"

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

void init_sensors()
{
	// todo: once sensors are connected, init them accordingly
}