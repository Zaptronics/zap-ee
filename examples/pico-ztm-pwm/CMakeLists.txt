# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "Set PICO_SDK_PATH before configuring this example")
endif()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(zapee_pico_ztm_pwm_example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# When building this example from inside a ZapEE checkout, use the checkout.
# A copied customer project would normally FetchContent ZapEE instead.
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_BINARY_DIR}/zapee EXCLUDE_FROM_ALL)

add_executable(zapee_pico_ztm_pwm main.c)
target_link_libraries(zapee_pico_ztm_pwm PRIVATE
    pico_stdlib
    hardware_i2c
    zap::pico_ztm_pwm
)
pico_add_extra_outputs(zapee_pico_ztm_pwm)
