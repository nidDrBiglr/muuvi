#ifndef BLE_H
#define BLE_H

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <autoconf.h>

#include "gatt.h"
#include "led.h"
#include "sensors.h"
#include "utils.h"

#define MEASUREMENT_INTERVAL K_SECONDS(30)
#define DEVICE_NAME_MAX_LEN  50

void init_ble();

#endif // BLE_H