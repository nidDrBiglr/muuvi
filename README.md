# Muuvi

This projects implements the Ruuvi advertisement protocol for a nrf52840dongle.

The idea is to connect a DHT11 or DHT22 sensors (or similar) to record environmental measurements and broadcast them using the [Ruuvi payload format](https://docs.ruuvi.com/communication/bluetooth-advertisements/data-format-5-rawv2).

At the moment, the firmware just consists of mocking the Ruuvi payload with valid randomly generated data values (hence the name Muuvi).
Currently, only temperature and humidity are mocked.

## Building & Flashing

Build the firmware using `west build -b nrf52840dongle/nrf52840 --pristine`.

Flashing has to be done via the nrf Connect for Desktop app (programmer), by entering DFU mode and flashing the hex file.

The generated hex file is located in `./build/muuvi/zephyr/zephyr.hex`.

For more details see the [nrf52840dongle documentation](https://docs.nordicsemi.com/bundle/ug_nrf52840_dongle/page/UG/nrf52840_Dongle/programming.html).

## Versions

- ncs: `v2.9.1`
- zephyr: `v3.7.99`
- nrf52480dongle hw rev: `2.1.1 (2023.19)`

## TODOs

- refactor to be event- and module based
- get sensor values from DHT11 or DHT22 via I2C
- add modules for sensor value collection
- add sequence diagram
- add hw wiring diagram
- add LED status indicator documentation
- add github actions workflow to build the hex file and add it as a release (very low prio)
