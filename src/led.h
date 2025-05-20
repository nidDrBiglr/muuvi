#ifndef LED_H
#define LED_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include <stdbool.h>

void init_leds(void);

void set_ble_adv_error_led(void);
void toggle_measurement_led(void);
void set_ble_init_error_led(void);

#endif