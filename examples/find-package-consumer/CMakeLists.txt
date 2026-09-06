# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20)
project(zapee_find_package_consumer LANGUAGES C)

find_package(ZapEE 0.5.0 CONFIG REQUIRED)

add_executable(zapee_find_package_consumer main.c)
target_link_libraries(zapee_find_package_consumer PRIVATE
    zap::ztm_pwm
    zap::ztm_adc
)
