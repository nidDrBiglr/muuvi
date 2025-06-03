cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(muuvi)

# Include files that are common for all modules
add_subdirectory(src/common)

# Include mandatory module source folders
add_subdirectory(src/modules/ble)
add_subdirectory(src/modules/events)
add_subdirectory(src/modules/led)
add_subdirectory(src/modules/sensing)
add_subdirectory(src/modules/timer)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
