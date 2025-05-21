#include "led.h"

#define LED1_NODE   DT_ALIAS(led0)
#define LED2_NODE_R DT_ALIAS(led1)
#define LED2_NODE_G DT_ALIAS(led2)
#define LED2_NODE_B DT_ALIAS(led3)

LOG_MODULE_REGISTER(led_ctrl);

static const struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led_2_r = GPIO_DT_SPEC_GET(LED2_NODE_R, gpios);
static const struct gpio_dt_spec led_2_g = GPIO_DT_SPEC_GET(LED2_NODE_G, gpios);
static const struct gpio_dt_spec led_2_b = GPIO_DT_SPEC_GET(LED2_NODE_B, gpios);

void init_leds(void)
{
	const struct gpio_dt_spec *leds[] = {&led_1, &led_2_r, &led_2_g, &led_2_b};
	size_t number_of_leds = sizeof(leds) / sizeof(leds[0]);
	LOG_INF("initializing %d LEDs...", number_of_leds);
	for (int i = 0; i < number_of_leds; i++) {
		if (device_is_ready(leds[i]->port)) {
			if (gpio_pin_configure_dt(leds[i], GPIO_OUTPUT_INACTIVE)) {
				LOG_WRN("led%d could not be configured", i);
			}
		} else {
			LOG_WRN("led%d is not ready", i);
		}
	}
}

// toggle the state of the green LED channel
void toggle_measurement_led()
{
	gpio_pin_toggle_dt(&led_2_g);
}

// turn on the red channel of the LED to indicate BLE init failure
void set_ble_init_error_led(void)
{
	gpio_pin_set_dt(&led_2_r, true);
}

// turn on the blue channel of the LED to indicate advertising failure
void set_ble_adv_error_led(void)
{
	gpio_pin_set_dt(&led_2_b, true);
}

void toggle_connection_led(void)
{
	gpio_pin_toggle_dt(&led_1);
}