#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <autoconf.h>

#include "led.h"
#include "sensors.h"
#include "utils.h"
#include "message_channel.h"

#define ADVERTISING_INTERVAL K_SECONDS(30)
#define DEVICE_NAME_MAX_LEN  50

LOG_MODULE_REGISTER(ble);
ZBUS_SUBSCRIBER_DEFINE(advertising, CONFIG_BLE_MESSAGE_QUEUE_SIZE);

static ssize_t dis_tx_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset);
BT_GATT_SERVICE_DEFINE(
	dis_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_BLE_ADV_MANUFACTURER_NAME),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_BLE_ADV_MODEL_NUMBER),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_BLE_ADV_SERIAL_NUMBER),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_BLE_ADV_FW_REV),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_HARDWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_BLE_ADV_HW_REV), );

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
	.options = BT_LE_ADV_OPT_CONNECTABLE,        // advertise as connectable
	.interval_min = BT_GAP_PER_ADV_SLOW_INT_MIN, // 1s
	.interval_max = BT_GAP_PER_ADV_SLOW_INT_MAX, // 1.2s
	.peer = NULL,
};

// client connected callback
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed, error: 0x%02x %s", err, bt_hci_err_to_str(err));
		int err = zbus_chan_pub(&LED_PATTERN_CHAN, &PATTERN_BLE_CONNECTION_FAILED,
					K_SECONDS(1));
		if (err) {
			LOG_ERR("zbus_chan_pub, error: %d", err);
			SEND_FATAL_ERROR();
		}
		return;
	}

	err = zbus_chan_pub(&LED_PATTERN_CHAN, &PATTERN_BLE_CONNECTED, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}

	struct bt_conn_info info;
	if (bt_conn_get_info(conn, &info)) {
		LOG_ERR("Connected to unknown device");
		return;
	}

	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str_without_type(info.le.dst, addr_str, sizeof(addr_str));

	LOG_INF("Connected to device '%s'", addr_str);
}

// client disconnected callback
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err = zbus_chan_pub(&LED_PATTERN_CHAN, &PATTERN_BLE_DISCONNECTED, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}

	struct bt_conn_info info;
	if (bt_conn_get_info(conn, &info)) {
		LOG_INF("Device disconnected, reason: 0x%02x %s", reason,
			bt_hci_err_to_str(reason));
		return;
	}

	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str_without_type(info.le.dst, addr_str, sizeof(addr_str));

	LOG_INF("Device '%s' disconnected, reason: 0x%02x %s", addr_str, reason,
		bt_hci_err_to_str(reason));
}

// advertisement data
static const struct bt_data advertising_data[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)) // measurements
};

// scan response data
static char device_name[DEVICE_NAME_MAX_LEN];
static struct bt_data scan_response[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, device_name, 0), // device name, set dynamically
	BT_DATA(BT_DATA_TX_POWER, "\x00", 1),           // tx power level (+0dBm), static
};

void update_advertisement_data(uint8_t *mfg_data, measurement_t *measurements)
{
	LOG_INF("updating BLE advertisement payload...");
	// update temperature in advertisement data
	mfg_data[3] = (measurements->temperature >> 8) & 0xFF;
	mfg_data[4] = measurements->temperature & 0xFF;
	// update humidity in advertisement data
	mfg_data[5] = (measurements->humidity >> 8) & 0xFF;
	mfg_data[6] = measurements->humidity & 0xFF;
	// update sequence number in advertisement data

	mfg_data[18] = (measurements->sequence_number >> 8) & 0xFF;
	mfg_data[19] = measurements->sequence_number & 0xFF;

	LOG_INF("updated advertising values: temperature: %f, humidity: %f",
		measurements->temperature * 0.005, measurements->humidity * 0.0025);
}

static ssize_t dis_tx_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	int err = zbus_chan_pub(&LED_PATTERN_CHAN, &PATTERN_DIS_TX_RECEIVED, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}
	char client_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	bt_addr_le_copy(&addr, bt_conn_get_dst(conn));
	bt_addr_le_to_str_without_type(&addr, client_addr, sizeof(client_addr));

	const char *str = attr->user_data;
	// todo: log characteristic UUID if possible
	LOG_INF("DIS TX: sending value '%s' to '%s'", str, client_addr);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, str, strlen(str));
}

// NUS RX callback handler
static void nus_rx_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	int err = zbus_chan_pub(&LED_PATTERN_CHAN, &PATTERN_NUS_RX_RECEIVED, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}
	char client_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	bt_addr_le_copy(&addr, bt_conn_get_dst(conn));
	bt_addr_le_to_str_without_type(&addr, client_addr, sizeof(client_addr));

	LOG_INF("NUS RX: received value '%s' from '%s'", data, client_addr);
}

void init_ble()
{
	int err;
	LOG_INF("initializing BLE module...");
	err = bt_enable(NULL);
	if (err) {
		err = zbus_chan_pub(&LED_PATTERN_CHAN, &PATTERN_BLE_INIT_FAILED, K_SECONDS(1));
		if (err) {
			LOG_ERR("zbus_chan_pub, error: %d", err);
			SEND_FATAL_ERROR();
		}
		LOG_ERR("BLE init failed, err %d", err);
		SEND_FATAL_ERROR();
		return;
	}

	// register connection callbacks
	struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};
	bt_conn_cb_register(&conn_callbacks);
	// init gatt services
	// NUS is required according to ruuvi specs to fully mock the behaviour
	// see https://docs.ruuvi.com/communication/bluetooth-connection/nordic-uart-service-nus
	struct bt_nus_cb nus_cb = {
		.received = nus_rx_cb,
	};
	// bt_nus_init(&nus_cb);

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
	scan_response[0].data_len = strlen(device_name);
	LOG_INF("visible as '%s' with address '%02X:%02X:%02X:%02X:%02X:%02X'", device_name,
		addr.a.val[5], addr.a.val[4], addr.a.val[3], addr.a.val[2], addr.a.val[1],
		addr.a.val[0]);

	LOG_INF("BLE initialized");
}

static measurement_t measurements;

static void advertising_task(void)
{
	const struct zbus_channel *chan;

	init_ble();
	// start advertising
	int err = bt_le_adv_start(&adv_params, advertising_data, ARRAY_SIZE(advertising_data),
				  scan_response, ARRAY_SIZE(scan_response));
	if (err) {
		err = zbus_chan_pub(&LED_PATTERN_CHAN, &PATTERN_BLE_ADVERTISING_FAILED,
				    K_SECONDS(1));
		if (err) {
			LOG_ERR("zbus_chan_pub, error: %d", err);
			SEND_FATAL_ERROR();
		}
		LOG_ERR("advertisement failed to start, err %d", err);
		SEND_FATAL_ERROR();
		return;
	}
	LOG_INF("starting initial advertising");

	while (!zbus_sub_wait(&advertising, &chan, K_FOREVER)) {
		if (&PAYLOAD_CHAN == chan) {
			int err = zbus_chan_read(&PAYLOAD_CHAN, &measurements, K_SECONDS(1));
			if (err) {
				LOG_ERR("zbus_chan_read, error: %d", err);
				SEND_FATAL_ERROR();
				return;
			}
			// update the advertisement data
			update_advertisement_data(mfg_data, &measurements);
			err = bt_le_adv_update_data(advertising_data, ARRAY_SIZE(advertising_data),
						    scan_response, ARRAY_SIZE(scan_response));
			if (err) {
				err = zbus_chan_pub(&LED_PATTERN_CHAN,
						    &PATTERN_BLE_ADVERTISING_FAILED, K_SECONDS(1));
				if (err) {
					LOG_ERR("zbus_chan_pub, error: %d", err);
					SEND_FATAL_ERROR();
				}
				LOG_ERR("failed to update advertisement params, err %d", err);
				return;
			}
			k_sleep(K_MSEC(100));
			LOG_INF("advertising data for sequence %d updated...",
				measurements.sequence_number);
			err = zbus_chan_pub(&LED_PATTERN_CHAN, &PATTERN_BLE_ADVERTISING,
					    K_SECONDS(1));
			if (err) {
				LOG_ERR("zbus_chan_pub, error: %d", err);
				SEND_FATAL_ERROR();
			}
		}
	}
}

K_THREAD_DEFINE(advertising_task_id, CONFIG_BLE_THREAD_STACK_SIZE, advertising_task, NULL, NULL,
		NULL, 3, 0, 0);