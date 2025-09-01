cmake_minimum_required(VERSION 3.13)

# Pico SDK einbinden (entweder via Submodule oder PICO_SDK_PATH Umgebungsvariable)
include(pico_sdk_import.cmake)

project(Heart-Communication-Device C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(Heart-Communication-Device
    src/main.c
    src/servo.c
    src/max30102.c
    src/mosfet.c
)

# <-- WICHTIG: Pfad zu deinen Headers
target_include_directories(Heart-Communication-Device PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(Heart-Communication-Device
    pico_stdlib
    hardware_pwm
    hardware_i2c
)

# USB als stdout (und UART aus)
pico_enable_stdio_usb(Heart-Communication-Device 1)
pico_enable_stdio_uart(Heart-Communication-Device 0)

# UF2, bin, elf erzeugen
pico_add_extra_outputs(Heart-Communication-Device)
