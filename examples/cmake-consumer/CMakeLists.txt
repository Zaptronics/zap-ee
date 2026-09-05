# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20)
project(zaptronics_consumer_example LANGUAGES C)

include(FetchContent)
FetchContent_Declare(
    zapee
    GIT_REPOSITORY https://github.com/Zaptronics/zap-ee.git
    GIT_TAG v0.4.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zapee)

add_executable(example main.c)
target_link_libraries(example PRIVATE zap::core)

# Once a module is extracted, a real application simply links it:
# target_link_libraries(example PRIVATE zap::ztm_pwm)
