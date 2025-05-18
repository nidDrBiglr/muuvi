#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/random/random.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define MEASUREMENT_INTERVAL K_SECONDS(30)
#define LED1_NODE DT_ALIAS(led0)
#define LED2_NODE_R DT_ALIAS(led1)
#define LED2_NODE_G DT_ALIAS(led2)
#define LED2_NODE_B DT_ALIAS(led3)

LOG_MODULE_REGISTER(muuvi);

static const struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led_2_r = GPIO_DT_SPEC_GET(LED2_NODE_R, gpios);
static const struct gpio_dt_spec led_2_g = GPIO_DT_SPEC_GET(LED2_NODE_G, gpios);
static const struct gpio_dt_spec led_2_b = GPIO_DT_SPEC_GET(LED2_NODE_B, gpios);

// see https://docs.ruuvi.com/communication/bluetooth-advertisements/data-format-5-rawv2
static uint8_t mfg_data[] = {
	// Company identifier (Ruuvi Innovations Ltd - 0x0499)
	0x99, 0x04,
	// Data format 5 (RAWv2)
	0x05,
	// Temperature in 0.005 degree steps
	// valid values: -32767 ... 32767
	0x12, 0xFC,
	// Humidity (16bit unsigned) in 0.0025% steps (0-163.83% range, though realistically 0-100%)
	// valid values: 0 ... 40000
	0x53, 0x94,
	// Atmospheric pressure (16bit unsigned) in 1 Pa units, with offset of -50 000 Pa
	// valid values: 0 ... 65534
	0xC3, 0x7C,
	// Acceleration-X (Most Significant Byte first)
	// valid values: -32767 ... 32767
	0x00, 0x00, // unused
	// Acceleration-Y (Most Significant Byte first)
	// valid values: -32767 ... 32767
	0x00, 0x00, // unused
	// Acceleration-Z (Most Significant Byte first)
	// valid values: -32767 ... 32767
	0x00, 0x00, // unused
	// Power info (11+5bit unsigned)
	// First 11 bits is the battery voltage above 1.6V, in millivolts (1.6V to 3.646V range)
	// Last 5 bits unsigned are the TX power above -40dBm, in 2dBm steps. (-40dBm to +20dBm range)
	// (0 ... 2046, 0 ... 30)
	// statically set to 3.646mV, since its powered by USB
	// 3.646mV = 2046 = 11111111110, +0dBm = 20 = 10100
	// 1111111111010100 = 0xFFD4
	0xFF, 0xD4, // static
	// Movement counter (8 bit unsigned), incremented by motion detection interrupts from accelerometer
	// valie values: 0 ... 254
	// unused
	0x00, // unused
	// Measurement sequence number (16 bit unsigned), each time a measurement is taken, this is incremented by one, used for measurement de-duplication.
	// Depending on the transmit interval, multiple packets with the same measurements can be sent, and there may be measurements that never were sent.
	// valid values: 0 ... 65534
	0xFF, 0xFF, // incremented each time a measurement is recorded, default value is 65535 to indicated "not available"
	// 48bit MAC address
	0xC7, 0x83, 0xB2, 0xBC, 0xC1, 0x53};

// advertisement parameters
static const struct bt_le_adv_param adv_params = {
	.id = BT_ID_DEFAULT,
	.options = BT_LE_ADV_OPT_USE_IDENTITY,
	.interval_min = BT_GAP_PER_ADV_SLOW_INT_MIN,
	.interval_max = BT_GAP_PER_ADV_SLOW_INT_MAX,
	.peer = NULL,
};

// advertisement data
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
};

// scan response data
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1), // device name
	BT_DATA(BT_DATA_TX_POWER, "\x00", 1),													  // tx power level (+0dBm), static
};

// sequence number, will be updated whenever a new measurement is recorded
// set initial value to 65535 to indicate "not available" if, for some reason, the sequence number is not updated
static uint16_t sequence_number = 65535;

// generate a random temperature value (-32767 to 32767, mapped to -163.835 to +163.835 degrees in 0.005 increments)
int16_t generate_random_temperature(void)
{
	return (sys_rand32_get() % 65536) - 32768;
}

// generate a random humidity value (0 to 40000, mapped to 0% to 100% in 0.0025 increments)
uint16_t generate_random_humidity(void)
{
	return sys_rand32_get() % 40001;
}

void update_advertisement_data(uint8_t *mfg_data)
{
	LOG_INF("collecting measurements...");
	// turn on the green channel of the LED to indicate measurement collecting
	gpio_pin_toggle_dt(&led_2_g);
	k_sleep(K_SECONDS(1));
	// get temperature value
	int16_t temperature = generate_random_temperature();
	uint16_t humidity = generate_random_humidity();
	// update temperature in advertisement data
	mfg_data[3] = (temperature >> 8) & 0xFF;
	mfg_data[4] = temperature & 0xFF;
	// update humidity in advertisement data
	mfg_data[5] = (humidity >> 8) & 0xFF;
	mfg_data[6] = humidity & 0xFF;
	// update sequence number in advertisement data
	sequence_number++;
	// reset sequence number if its > 65534, as the max allowed value is 65534
	// 65535 is reserved for "not available"
	if (sequence_number > 65534)
	{
		// reset sequence number, if greater than allowed value
		// sequence number will be reset every ~23 days if a measurement is advertised every 30 seconds...
		sequence_number = 0;
	}
	mfg_data[18] = (sequence_number >> 8) & 0xFF;
	mfg_data[19] = sequence_number & 0xFF;

	LOG_INF("updated advertising values: temperature: %f, humidity: %f", temperature * 0.005, humidity * 0.0025);
	// turn off the green channel of the LED
	gpio_pin_toggle_dt(&led_2_g);
}

int init_leds(void)
{
	int err;
	// setup the green LED (LED1)
	if (!gpio_is_ready_dt(&led_1))
	{
		return 1;
	}
	err = gpio_pin_configure_dt(&led_1, GPIO_OUTPUT_INACTIVE);
	if (err)
	{
		return err;
	}
	// setup the red channel of LED2
	if (!gpio_is_ready_dt(&led_2_r))
	{
		return 1;
	}
	err = gpio_pin_configure_dt(&led_2_r, GPIO_OUTPUT_INACTIVE);
	if (err)
	{
		return err;
	}
	// setup the green channel of LED2
	if (!gpio_is_ready_dt(&led_2_g))
	{
		return 1;
	}
	err = gpio_pin_configure_dt(&led_2_g, GPIO_OUTPUT_INACTIVE);
	if (err)
	{
		return err;
	}
	// setup the blue channel of LED2
	if (!gpio_is_ready_dt(&led_2_b))
	{
		return 1;
	}
	err = gpio_pin_configure_dt(&led_2_b, GPIO_OUTPUT_INACTIVE);
	if (err)
	{
		return err;
	}
	// led setup successful
	return 0;
}

int main(void)
{
	LOG_INF("---- starting Ruuvi for nrf52840dongle ----");
	int err;

	LOG_INF("initializing LEDs...");
	err = init_leds();
	if (err)
	{
		LOG_ERR("LED init failed, err %d", err);
		return 0;
	}

	LOG_INF("enabling BLE module...");
	err = bt_enable(NULL);
	if (err)
	{
		// turn on the red channel of the LED to indicate BLE init failure
		gpio_pin_toggle_dt(&led_2_r);
		LOG_ERR("BLE init failed, err %d", err);
		return 0;
	}

	LOG_INF("BLE initialized, starting advertisement cycle...");
	while (true)
	{
		// stop advertising before updating data
		bt_le_adv_stop();
		// update the advertisement data
		update_advertisement_data(mfg_data);
		// restart advertising with updated data
		err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		if (err)
		{
			// turn on the blue channel of the LED to indicate advertising failure
			gpio_pin_toggle_dt(&led_2_b);
			LOG_ERR("advertisement failed to start, err %d", err);
			return 0;
		}
		LOG_INF("advertising sequence %d", sequence_number);
		// sleep until next measurement interval
		k_sleep(MEASUREMENT_INTERVAL);
	}

	return 0;
}
