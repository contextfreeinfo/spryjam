cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD adafruit_fruit_jam CACHE STRING "Board type")

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(spryjam C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src)
add_subdirectory(lib/pico_hdmi)

add_executable(spryjam
    src/main.c
    src/draw.c
)

# target_compile_definitions(spryjam PRIVATE VIDEO_MODE_1280x720)
# target_compile_definitions(pico_hdmi PRIVATE VIDEO_MODE_1280x720)

# 720p overclocks sys_clk to 372 MHz, at which XIP flash reads corrupt (the
# boot-clock QMI timing no longer samples correctly) and the core hard-faults.
# Execute from RAM so flash is only read at the safe boot clock. Robust and
# flash-chip-independent.
# pico_set_binary_type(spryjam copy_to_ram)

target_link_libraries(spryjam
    pico_stdlib
    pico_multicore
    hardware_clocks
    hardware_dma
    hardware_irq
    pico_hdmi
)

# Enable USB output, disable UART
pico_enable_stdio_usb(spryjam 1)
pico_enable_stdio_uart(spryjam 0)

pico_add_extra_outputs(spryjam)
