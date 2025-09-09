# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(max30102_test C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(max30102_test 
    src/clientTCP.c
    src/exe_servo_mosfet.c
    main.c
    src/max30102.c
    src/mosfet.c
    src/serverTCP.c
    src/servo.c)

pico_set_program_name(max30102_test "max30102_test")
pico_set_program_version(max30102_test "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(max30102_test 0)
pico_enable_stdio_usb(max30102_test 1)

# Add the standard library to the build
target_link_libraries(max30102_test 
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_i2c
    hardware_pwm
    hardware_gpio
)



# Add the standard include files to the build
target_include_directories(max30102_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_SDK_PATH}/stdlib
    ${PICO_SDK_PATH}/stdint
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${PICO_SDK_PATH}/hardware/pwm
    ${PICO_SDK_PATH}/hardware/i2c
    ${PICO_SDK_PATH}/hardware/gpio
)


pico_add_extra_outputs(max30102_test)

