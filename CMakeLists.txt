cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD adafruit_fruit_jam CACHE STRING "Board type")
set(PICO_PIO_USB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/Pico-PIO-USB)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(spryjam C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src)
add_subdirectory(lib/pico_hdmi)
include(lib/rio/library.cmake)

# From wili8jam.
# --- USB Host library (PIO-USB, HID, XInput, CDC, MSC) ---
set(FWUSBHOST_TUSB_DEBUG_LEVEL 2)  # output goes to ring buffer, dumped via 'info'
add_subdirectory(lib/wili8jam/usb-host)

add_executable(spryjam
    src/main.c
    src/draw.c
    src/font.c
    src/input.c
    src/input-cpp.cpp
    src/mem.c
    src/run.c
    src/sdcard.c
    src/text.c
    # TinyUSB host stack — must be compiled in the executable (not fwUSBHost lib)
    # to avoid duplicate symbols with the device stack from pico_stdio_usb
    ${FWUSBHOST_TINYUSB_HOST_SOURCES}
)

target_include_directories(spryjam PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# target_compile_definitions(spryjam PRIVATE VIDEO_MODE_1280x720)
# target_compile_definitions(pico_hdmi PRIVATE VIDEO_MODE_1280x720)
target_compile_definitions(spryjam PRIVATE MODE_HSTX_CLK_DIV=2)
target_compile_definitions(pico_hdmi PRIVATE MODE_HSTX_CLK_DIV=2)

# 720p overclocks sys_clk to 372 MHz, at which XIP flash reads corrupt (the
# boot-clock QMI timing no longer samples correctly) and the core hard-faults.
# Execute from RAM so flash is only read at the safe boot clock. Robust and
# flash-chip-independent.
# pico_set_binary_type(spryjam copy_to_ram)

target_link_libraries(spryjam
    fwUSBHost
    hardware_clocks
    hardware_dma
    hardware_irq
    hardware_pio
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    pico_hdmi
    pico_multicore
    pico_stdlib
    rio-lib
)

target_compile_definitions(spryjam PRIVATE
    PICO_STACK_SIZE=0x1000
    # PicoHDMI uses lots of scratch_x, so that impacts stack options.
    PICO_CORE1_STACK_SIZE=0x800
    # PICO_CRT0_ALLOCATE_SPACERS=0
)

target_compile_options(spryjam PRIVATE
    # -Wall -Wextra -Wpedantic -pedantic-errors -Werror
    -fstack-usage
)

# Enable USB output, disable UART
pico_enable_stdio_usb(spryjam 1)
pico_enable_stdio_uart(spryjam 0)

pico_add_extra_outputs(spryjam)
