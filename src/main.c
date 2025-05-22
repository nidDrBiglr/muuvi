#include <zephyr/logging/log.h>

#include "ble.h"
#include "led.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
	LOG_INF("---- starting Ruuvi for nrf52840dongle ----");

	init_leds();

	init_ble();

	return 0;
}
