#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#pragma once

#include <zephyr/kernel.h>

#include "../led/led.h"
#include "../sensing/sensors.h"

typedef enum {
    SENSOR_SAMPLE_EVENT,
    SENSOR_DATA_READY_EVENT,
    LED_UPDATE_EVENT
} app_event_type_t;

typedef struct {
    app_event_type_t type;
    union {
        measurement_t measurement;
        led_pattern_t led_pattern;
    } data;
} app_event_t;

int event_bus_post(app_event_t *evt);
int event_bus_get(app_event_t *evt, k_timeout_t timeout);

#endif // EVENT_BUS_H