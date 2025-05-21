#include "gatt_services.h"

LOG_MODULE_REGISTER(gatt);

// NUS RX callback handler
static void nus_rx_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	char client_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr;
	bt_addr_le_copy(&addr, bt_conn_get_dst(conn));
	bt_addr_le_to_str(&addr, client_addr, sizeof(client_addr));
	set_led_pattern(&PATTERN_NUS_RX_RECEIVED);

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