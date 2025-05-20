#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/random/random.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <autoconf.h>

#include "led.h"

#define MEASUREMENT_INTERVAL K_SECONDS(30)
#define DEVICE_NAME_MAX_LEN  50

LOG_MODULE_REGISTER(muuvi);

// see https://docs.ruuvi.com/communication/bluetooth-advertisements/data-format-5-rawv2
// all payload values are initialized with their "not available" values
static uint8_t mfg_data[] = {
	// Company identifier (Ruuvi Innovations Ltd - 0x0499)
	0x99, 0x04,
	// Data format 5 (RAWv2)
	0x05,
	// Temperature in 0.005 degree steps
	// valid values: -32767 ... 32767, "not available": 0x8000
	0x80, 0x00,
	// Humidity (16bit unsigned) in 0.0025% steps (0-163.83% range, though realistically 0-100%)
	// valid values: 0 ... 40000, "not available": 0xFFFF
	0xFF, 0xFF,
	// Atmospheric pressure (16bit unsigned) in 1 Pa units, with offset of -50 000 Pa
	// valid values: 0 ... 65534, "not available": 0xFFFF
	0xFF, 0xFF,
	// Acceleration-X (Most Significant Byte first)
	// valid values: -32767 ... 32767, "not available": 0x8000
	0x80, 0x00,
	// Acceleration-Y (Most Significant Byte first)
	// valid values: -32767 ... 32767, "not available": 0x8000
	0x80, 0x00,
	// Acceleration-Z (Most Significant Byte first)
	// valid values: -32767 ... 32767, "not available": 0x8000
	0x80, 0x00,
	// Power info (11+5bit unsigned)
	// First 11 bits is the battery voltage above 1.6V, in millivolts (1.6V to 3.646V range)
	// Last 5 bits unsigned are the TX power above -40dBm - +20dB,, in 2dBm steps.
	// valid values: 0 ... 2046 and 0 ... 30 respectively
	// statically set to 3.3V, since its powered by USB
	// 3.3V = 1700 = 11010100100, +0dBm = 20 = 10100 => 1101010010010100_b = 0xD494_h
	0xD4, 0x94, // static
	// Movement counter (8 bit unsigned), incremented by motion detection interrupts from
	// accelerometer
	// valie values: 0 ... 254, "not available": 0xFF
	0xFF, // unused
	// Measurement sequence number (16 bit unsigned), each time a measurement is taken, this is
	// incremented by one, used for measurement de-duplication.
	// Depending on the transmit interval, multiple packets with the same measurements can be
	// sent, and there may be measurements that never were sent.
	// valid values: 0 ... 65534, "not available": 0xFFFF
	0xFF, 0xFF, // incremented each time a measurement is recorded
	// any valid MAC address
	// will be dynamically set by the firmware after the BLE module is initialized
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// advertisement parameters
static const struct bt_le_adv_param adv_params = {
	.id = BT_ID_DEFAULT,
	.options = BT_LE_ADV_OPT_USE_IDENTITY,       // use HW address
	.interval_min = BT_GAP_PER_ADV_SLOW_INT_MIN, // 1s
	.interval_max = BT_GAP_PER_ADV_SLOW_INT_MAX, // 1.2s
	.peer = NULL,
};

// advertisement data
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)) // measurements
};

// scan response data
static char device_name[DEVICE_NAME_MAX_LEN];
static struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, device_name, 0), // device name, set dynamically
	BT_DATA(BT_DATA_TX_POWER, "\x00", 1),           // tx power level (+0dBm), static
};

// sequence number, will be updated whenever a new measurement is recorded
// set initial value to 65535 to indicate "not available" if, for some reason, the sequence number
// is not updated
static uint16_t sequence_number = 65535;

// boolean to indicate advertising state
static bool is_advertising = false;

// generate a random temperature value (-32767 to 32767, mapped to -163.835 to +163.835 degrees in
// 0.005 increments)
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
	toggle_measurement_led();
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
	if (sequence_number > 65534) {
		// reset sequence number, if greater than allowed value
		// sequence number will be reset every ~23 days with advertisements every 30 seconds
		sequence_number = 0;
	}
	mfg_data[18] = (sequence_number >> 8) & 0xFF;
	mfg_data[19] = sequence_number & 0xFF;

	LOG_INF("updated advertising values: temperature: %f, humidity: %f", temperature * 0.005,
		humidity * 0.0025);

	toggle_measurement_led();
}

int main(void)
{
	LOG_INF("---- starting Ruuvi for nrf52840dongle ----");
	int err;

	init_leds();

	LOG_INF("initializing BLE module...");
	err = bt_enable(NULL);
	if (err) {
		set_ble_init_error_led();
		LOG_ERR("BLE init failed, err %d", err);
		return 0;
	}

	// dynamically set the BLE MAC address of the dongle in the payload
	bt_addr_le_t addr;
	size_t count = 1;
	bt_id_get(&addr, &count);
	// set mac address of payload bytes 20 - 25 from addr.a (neds to be in reverse order)
	for (int i = 0; i < 6; i++) {
		mfg_data[20 + i] = addr.a.val[5 - i];
	}
	// set the device name and append the last 2 bytes of the MAC address to the name
	snprintf(device_name, sizeof(device_name), "%s %02X%02X", CONFIG_BT_DEVICE_NAME,
		 addr.a.val[1], addr.a.val[0]);
	// update the length of the device name scan response
	sd[0].data_len = strlen(device_name);
	LOG_INF("visible as '%s' with address '%02X:%02X:%02X:%02X:%02X:%02X'", device_name,
		addr.a.val[5], addr.a.val[4], addr.a.val[3], addr.a.val[2], addr.a.val[1],
		addr.a.val[0]);

	LOG_INF("BLE initialized, starting advertisement cycle...");
	while (true) {
		// stop advertising before updating the data
		if (is_advertising) {
			LOG_INF("stopping advertising sequence %d...", sequence_number);
			bt_le_adv_stop();
			is_advertising = false;
		}
		// update the advertisement data
		update_advertisement_data(mfg_data);
		// restart advertising with updated data
		err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		if (err) {
			set_ble_adv_error_led();
			LOG_ERR("advertisement failed to start, err %d", err);
			return 0;
		}
		is_advertising = true;
		LOG_INF("advertising sequence %d started...", sequence_number);
		// sleep until next measurement interval
		k_sleep(MEASUREMENT_INTERVAL);
	}

	return 0;
}
