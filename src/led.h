/**
 * @file
 * @brief LED controls for the nrf52840 RGB and static green LED.
 *
 * The below table explains the different patterns in more detail.
 *
 * | Pattern                        | Color     | Description                       | Duration  	|
 * | ------------------------------ | --------- | --------------------------------- | -------------	|
 * | PATTERN_BLE_INIT_FAILED        | Red       | Solid 2s every 5s                 | Forever   	|
 * | PATTERN_BLE_ADVERTISING_FAILED | Red       | Two quick flashes, then pause     | Forever   	|
 * | PATTERN_COLLECTING_SENSOR      | Yellow    | Pulsing                           | 100ms/cycle	|
 * | PATTERN_BLE_ADVERTISING        | Cyan      | Smooth fade in/out                | 4000ms		|
 * | PATTERN_BLE_CONNECTED          | Red-Blue  | Blue to blue flash transition     | 400ms     	|
 * | PATTERN_BLE_CONNECTION_FAILED  | Red       | Three quick flashes               | 1000ms   		|
 * | PATTERN_BLE_DISCONNECTED       | Blue-Red	| Red to blue flash transition     	| 200ms     	|
 * | PATTERN_DIS_TX_RECEIVED	    | Pink 		| Pulsing					     	| 200ms     	|
 * | PATTERN_NUS_RX_RECEIVED        | Purple    | Pulsing		                    | 400ms     	|
 *
 * NOTE: PATTERN_BLE_CONNECTED will also turn on LED1, the PATTERN_BLE_DISCONNECTED will
 * turn it off again.
 *
 */

#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

#include <stdbool.h>

#define PWM_PERIOD_USEC PWM_USEC(2000)
#define STEP_DURATION   K_MSEC(50)

/**
 * @brief PWM step function to control PWM LED patterns
 *
 * @return false, if the pattern has completed
 */
typedef bool (*led_pattern_fn_t)(int *step);

typedef struct {
	const char *name;
	led_pattern_fn_t step_fn;
} led_pattern_t;

extern const led_pattern_t PATTERN_BLE_INIT_FAILED;
extern const led_pattern_t PATTERN_BLE_ADVERTISING_FAILED;
extern const led_pattern_t PATTERN_COLLECTING_SENSOR;
extern const led_pattern_t PATTERN_BLE_ADVERTISING;
extern const led_pattern_t PATTERN_BLE_CONNECTED;
extern const led_pattern_t PATTERN_BLE_CONNECTION_FAILED;
extern const led_pattern_t PATTERN_BLE_DISCONNECTED;
extern const led_pattern_t PATTERN_DIS_TX_RECEIVED;
extern const led_pattern_t PATTERN_NUS_RX_RECEIVED;

/**
 * @brief init the LEDs. Must be invoked in order to show any patterns
 */
void init_leds(void);

/**
 * @brief set the pattern to be shown.
 *
 * In order to reset the current pattern or turn the LEDs off, invoke with NULL.
 *
 * NOTE: setting a pattern while another one is already running will override it.
 * It also might sometimes cause deadlocks... its really up to chance
 *
 */
void set_led_pattern(const led_pattern_t *pattern);

#ifdef __cplusplus
}
#endif

#endif // LED_H