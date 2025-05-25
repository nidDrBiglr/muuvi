#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ble/ble.h"
#include "led/led.h"
#include "controller/controller.h"
#include "sensing/sensors.h"

LOG_MODULE_REGISTER(main);

#define STACK_SIZE 1024
#define PRIORITY   5

K_THREAD_STACK_DEFINE(controller_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(sensor_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(ble_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(led_stack, STACK_SIZE);

static struct k_thread controller_thread_data;
static struct k_thread sensor_thread_data;
static struct k_thread ble_thread_data;
static struct k_thread led_thread_data;

void controller_thread(void *a, void *b, void *c)
{
	controller_run();
}

void sensor_thread(void *a, void *b, void *c)
{
	sensors_run();
}

void ble_thread(void *a, void *b, void *c)
{
	ble_run();
}

void led_thread(void *a, void *b, void *c)
{
	led_run();
}

int main(void)
{
	LOG_INF("---- starting Ruuvi Firmware for nrf52840dongle ----");
    ble_init();
    led_init();
    sensors_init();
    controller_init();

	k_thread_create(&controller_thread_data, controller_stack, STACK_SIZE, controller_thread,
			NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&sensor_thread_data, sensor_stack, STACK_SIZE, sensor_thread, NULL, NULL,
			NULL, PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&ble_thread_data, ble_stack, STACK_SIZE, ble_thread, NULL, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&led_thread_data, led_stack, STACK_SIZE, led_thread, NULL, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);

	return 0;
}
