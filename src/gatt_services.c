#include "gatt_services.h"

#define MODEL_NUMBER      "RuuviTag"
#define SERIAL_NUMBER     "F9C3B50276D1"
#define MANUFACTURER_NAME "Ruuvi Innovations Ltd"
#define HARDWARE_REVISION "B"
#define FIRMWARE_REVISION "MuuviFW 3.31.1"
#define SOFTWARE_REVISION ""

LOG_MODULE_REGISTER(gatt);

// generic DIS string read callback handler
static ssize_t dis_read_string_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	// get the client address
	char client_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	bt_addr_le_copy(&addr, bt_conn_get_dst(conn));
	bt_addr_le_to_str(&addr, client_addr, sizeof(client_addr));
	// log what has been read
	const char *dis_value = attr->user_data;
	LOG_INF("DIS: sending '%s' to %s", dis_value, client_addr);
	// return the DIS value
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_value, strlen(dis_value));
}

// DIS characteristics, which are required to retrieve FW version for full mocking of the behaviour
// todo: can this be done via kconfig?
BT_GATT_SERVICE_DEFINE(
	dis_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_read_string_cb, NULL, MANUFACTURER_NAME),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_read_string_cb, NULL, MODEL_NUMBER),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_read_string_cb, NULL, SERIAL_NUMBER),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_HARDWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_read_string_cb, NULL, HARDWARE_REVISION),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       dis_read_string_cb, NULL, FIRMWARE_REVISION), );

// nus callback handler to print if a client tries to write to the rx characteristic
// nothing will be done with the data though, but it might be interesting to see
static void nus_rx_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	char client_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	bt_addr_le_copy(&addr, bt_conn_get_dst(conn));
	bt_addr_le_to_str(&addr, client_addr, sizeof(client_addr));

	LOG_INF("NUS RX: received '%s' from '%s'", data, client_addr);
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