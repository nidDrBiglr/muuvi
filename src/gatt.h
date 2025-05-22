#ifndef GATT_H
#define GATT_H

#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "led.h"

void init_gatt_services(void);

#endif // GATT_H