#ifndef SENSORS_H
#define SENSORS_H

#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

typedef struct {
	int16_t temperature;
	uint16_t humidity;
} measurement_t;

void init_sensors(void);

void read_sensor_values(measurement_t *measurements);

#endif // SENSORS_H