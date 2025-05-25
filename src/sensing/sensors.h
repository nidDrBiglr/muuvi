#ifndef SENSORS_H
#define SENSORS_H

#pragma once

#include <zephyr/kernel.h>

typedef struct {
	int16_t temperature;
	uint16_t humidity;
} measurement_t;

void sensors_init(void);
void sensors_run(void);


#endif // SENSORS_H