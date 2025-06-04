#include "gatt.h"

LOG_MODULE_REGISTER(gatt);

static ssize_t dis_tx_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	set_led_pattern(&PATTERN_DIS_TX_RECEIVED);
	char client_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	bt_addr_le_copy(&addr, bt_conn_get_dst(conn));
	bt_addr_le_to_str_without_type(&addr, client_addr, sizeof(client_addr));

	const char *str = attr->user_data;
	// todo: log characteristic UUID if possible
	LOG_INF("DIS TX: sending value '%s' to '%s'", str, client_addr);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, str, strlen(str));
}

BT_GATT_SERVICE_DEFINE(
	dis_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_MANUFACTURER_NAME),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_MODEL_NUMBER),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_SERIAL_NUMBER),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_FW_REV),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_HARDWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_tx_cb, NULL, CONFIG_HW_REV), );

// NUS RX callback handler
static void nus_rx_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	set_led_pattern(&PATTERN_NUS_RX_RECEIVED);
	char client_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	bt_addr_le_copy(&addr, bt_conn_get_dst(conn));
	bt_addr_le_to_str_without_type(&addr, client_addr, sizeof(client_addr));

	LOG_INF("NUS RX: received value '%s' from '%s'", data, client_addr);
}

void init_gatt_services(void)
{
	// NUS is required according to ruuvi specs to fully mock the behaviour
	// see https://docs.ruuvi.com/communication/bluetooth-connection/nordic-uart-service-nus
	struct bt_nus_cb nus_cb = {
		.received = nus_rx_cb,
	};
	bt_nus_init(&nus_cb);
}