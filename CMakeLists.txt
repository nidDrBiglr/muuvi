cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(muuvi)

add_subdirectory(src/ble)
add_subdirectory(src/controller)
add_subdirectory(src/events)
add_subdirectory(src/led)
add_subdirectory(src/sensing)
add_subdirectory(src/utils)

target_sources(app PRIVATE src/main.c)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
