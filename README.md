# nrf52840dongle Ruuvi

This projects implements the Ruuvi advertisement protocol for a nrf52840dongle.

The idea is to connect a DHT11 or DHT22 sensor to measure environmental values and broadcast them using the Ruuvi payload format.

At the moment, the firmware just consists of mocking the Ruuvi payload with valid randomly generated data values.
Currently, only temperature and humidity are mocked.

## Building & Flashing

Build the firmware using `west build -b nrf52840dongle/nrf52840 --pristine`.

Flashing has to be done via the nrf Connect for Desktop app (programmer), by entering DFU mode.
For more details see he [nrf52840dongle documentation](https://docs.nordicsemi.com/bundle/ug_nrf52840_dongle/page/UG/nrf52840_Dongle/programming.html).

## Versions

- ncs: v2.8.0
- zephyr: v3.7.99
- nrf52480dongle hw rev: 2.1.1 (2023.19) 

## TODOs

- get sensor values from DHT11 or DHT22 via I2C
- add modules for DHT11/22 handling, BLE advertising and LED handling
