#ifndef SENSORS_H
#define SENSORS_H

#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include "message_channel.h"

void read_sensor_values(measurement_t *measurements);

#endif // SENSORS_H