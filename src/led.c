#include "led.h"

LOG_MODULE_REGISTER(led_ctrl);

// GPIO LEDs
static const struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct gpio_dt_spec *gpio_leds[] = {&led_1};
static size_t number_of_gpio_leds = sizeof(gpio_leds) / sizeof(gpio_leds[0]);
// PWM LEDs
static const struct pwm_dt_spec red_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
static const struct pwm_dt_spec green_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
static const struct pwm_dt_spec blue_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

static const struct pwm_dt_spec *pwm_leds[] = {&red_pwm_led, &green_pwm_led, &blue_pwm_led};
static size_t number_of_pwm_leds = sizeof(pwm_leds) / sizeof(pwm_leds[0]);
// current pattern and step
static const led_pattern_t *current_pattern = NULL;
static int pattern_step = 0;
// timer object to handle pulsing
static struct k_timer led_timer;
// indicator whether leds have been initialized
static bool leds_initialized = false;

// utility function to set RGB values
void set_rgb(uint32_t r, uint32_t g, uint32_t b)
{
	pwm_set_pulse_dt(&red_pwm_led, r);
	pwm_set_pulse_dt(&green_pwm_led, g);
	pwm_set_pulse_dt(&blue_pwm_led, b);
}

// utility functions to turn off the LEDs
void pwm_off()
{
	set_rgb(0, 0, 0);
}

void gpio_off()
{
	gpio_pin_set_dt(&led_1, false);
}

void all_off()
{
	pwm_off();
	gpio_off();
}

// handle timer events, stops the timer if the pattern has finished
void led_timer_handler(struct k_timer *timer)
{
	if (current_pattern && current_pattern->step_fn) {
		bool running = current_pattern->step_fn(&pattern_step);
		if (!running) {
			k_timer_stop(&led_timer);
		}
	}
}

// sets and triggers the given led pattern
// todo: can cause a dead lock, if timing is unlucky... add a locking mechanism or cancel timer
// immediately when invoked?
void set_led_pattern(const led_pattern_t *pattern)
{
	if (leds_initialized) {
		current_pattern = pattern;
		pattern_step = 0;

		if (pattern) {
			LOG_DBG("activating LED Pattern %s", pattern->name);
			k_timer_start(&led_timer, K_NO_WAIT, STEP_DURATION);
		} else {
			LOG_DBG("turning all LEDs off");
			k_timer_stop(&led_timer);
			all_off();
		}
	} else {
		LOG_WRN("some of the GPIO or PWM LEDs could not be initialized, ignoring...");
	}
}

void init_leds(void)
{
	LOG_INF("initializing GPIO %d LEDs...", number_of_gpio_leds);
	for (int i = 0; i < number_of_gpio_leds; i++) {
		if (device_is_ready(gpio_leds[i]->port)) {
			if (gpio_pin_configure_dt(gpio_leds[i], GPIO_OUTPUT_INACTIVE)) {
				LOG_WRN("GPIO led%d could not be configured", i);
				return;
			}
		} else {
			LOG_WRN("GPIO led%d is not ready, aborting...", i);
			return;
		}
	}
	LOG_INF("GPIO LEDs initialized, checking readiness of %d PWM LEDs...", number_of_pwm_leds);
	for (int i = 0; i < number_of_pwm_leds; i++) {
		if (!pwm_is_ready_dt(pwm_leds[i])) {
			LOG_WRN("PWM led%d is not ready, aborting...", i);
			return;
		}
	}
	LOG_INF("All %d PWM LEDs are ready", number_of_pwm_leds);
	leds_initialized = true;

	// initialize the timer to handle PWM pulsing
	k_timer_init(&led_timer, led_timer_handler, NULL);
}

// error pattern for BLE init failure
bool pattern_ble_init_failed_step(int *step)
{
	switch (*step) {
	case 0:
		set_rgb(PWM_PERIOD_USEC, 0, 0);
		break;
	case 40:
		pwm_off();
		break;
	case 140:
		*step = -1;
		break;
	}
	(*step)++;
	return true;
}

const led_pattern_t PATTERN_BLE_INIT_FAILED = {
	.name = "BLE Init Failed",
	.step_fn = pattern_ble_init_failed_step,
};

// error pattern for ble advertising failure
bool pattern_ble_advertising_failed_step(int *step)
{
	switch (*step) {
	case 0:
	case 8:
		set_rgb(PWM_PERIOD_USEC, 0, 0); // red blink
		break;
	case 4:
	case 12:
		pwm_off();
		break;
	case 40:
		*step = -1; // reset after pause
		break;
	}
	(*step)++;
	return true;
}

const led_pattern_t PATTERN_BLE_ADVERTISING_FAILED = {
	.name = "BLE Advertising Failed",
	.step_fn = pattern_ble_advertising_failed_step,
};

// pattern for collecting sensor data
bool pattern_collecting_sensor_step(int *step)
{
	if (*step % 2 == 1) {
		set_rgb(PWM_PERIOD_USEC, PWM_PERIOD_USEC, 0); // yellow on
	} else {
		pwm_off();
	}
	(*step)++;
	return true;
}

const led_pattern_t PATTERN_COLLECTING_SENSOR = {
	.name = "Collecting Sensor Measurements",
	.step_fn = pattern_collecting_sensor_step,
};

// pattern for advertising data change
bool pattern_ble_advertising_step(int *step)
{
	int phase = *step % 40;
	uint32_t duty;
	// calculate duty cycle for breathing pattern
	if (phase < 20) {
		duty = (PWM_PERIOD_USEC * phase) / 20;
	} else {
		duty = (PWM_PERIOD_USEC * (40 - phase)) / 20;
	}

	set_rgb(0, duty, duty / 2);
	if (*step == 80) {
		pwm_off();
		return false;
	}
	(*step)++;
	return true;
}

const led_pattern_t PATTERN_BLE_ADVERTISING = {
	.name = "BLE Advertising Updated",
	.step_fn = pattern_ble_advertising_step,
};

// pattern for successful ble connection
bool pattern_ble_connected_step(int *step)
{
	switch (*step) {
	case 0:
	case 2:
	case 4:
	case 6:
		set_rgb(0, 0, PWM_PERIOD_USEC);
		break;
	case 7:
		pwm_off();
		gpio_pin_toggle_dt(&led_1);
		return false;
	default:
		pwm_off();
	}
	(*step)++;
	return true;
}

const led_pattern_t PATTERN_BLE_CONNECTED = {
	.name = "BLE Connected",
	.step_fn = pattern_ble_connected_step,
};

// pattern for ble connection failure
static bool pattern_ble_connection_failed_step(int *step)
{
	switch (*step) {
	case 0:
	case 8:
	case 16:
		set_rgb(PWM_PERIOD_USEC, 0, 0);
		break;
	case 4:
	case 12:
	case 20:
		all_off();
		return false;
	}
	(*step)++;
	return true;
}

const led_pattern_t PATTERN_BLE_CONNECTION_FAILED = {
	.name = "BLE Connection Failed",
	.step_fn = pattern_ble_connection_failed_step,
};

// pattern for ble disconnection
bool pattern_ble_disconnected_step(int *step)
{
	switch (*step) {
	case 0:
		set_rgb(0, 0, PWM_PERIOD_USEC);
		break;
	case 2:
		set_rgb(PWM_PERIOD_USEC, 0, 0);
		break;
	case 4:
		all_off();
		return false;
	}
	(*step)++;
	return true;
}

const led_pattern_t PATTERN_BLE_DISCONNECTED = {
	.name = "BLE Disconnected",
	.step_fn = pattern_ble_disconnected_step,
};

// pattern for DIS TX received
bool pattern_dis_tx_received_step(int *step)
{

	switch (*step) {
	case 0:
	case 2:
		set_rgb(PWM_PERIOD_USEC, PWM_PERIOD_USEC / 4, PWM_PERIOD_USEC / 2);
		break;
	case 3:
		pwm_off();
		return false;
	default:
		pwm_off();
	}
	(*step)++;
	return true;
}

const led_pattern_t PATTERN_DIS_TX_RECEIVED = {
	.name = "DIS TX Command Received",
	.step_fn = pattern_dis_tx_received_step,
};

// pattern for NUS RX received
bool pattern_nus_rx_received_step(int *step)
{

	switch (*step) {
	case 0:
	case 2:
	case 4:
	case 6:
		set_rgb(PWM_PERIOD_USEC, 0, PWM_PERIOD_USEC);
		break;
	case 7:
		pwm_off();
		return false;
	default:
		pwm_off();
	}
	(*step)++;
	return true;
}

const led_pattern_t PATTERN_NUS_RX_RECEIVED = {
	.name = "NUS RX Command Received",
	.step_fn = pattern_nus_rx_received_step,
};