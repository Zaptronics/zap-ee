# SPDX-License-Identifier: Apache-2.0

zaptronics_add_component(
    NAME pico
    SOURCES pico.c
    PUBLIC_DEPS
        zap::i2c
        zap::spi
        hardware_i2c
        hardware_spi
        pico_sync
    PRIVATE_DEPS
        pico_time
        hardware_gpio
    VENDOR_EXTENSIONS
)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../modules/ztm_pwm/CMakeLists.txt")
zaptronics_add_component(
    NAME pico_ztm_pwm
    SOURCES ztm_pwm.c
    PUBLIC_DEPS
        zap::pico
        zap::ztm_pwm
    PRIVATE_DEPS
        hardware_gpio
    VENDOR_EXTENSIONS
)
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../modules/ztm_adc/CMakeLists.txt")
zaptronics_add_component(
    NAME pico_ztm_adc
    SOURCES ztm_adc.c
    PUBLIC_DEPS
        zap::pico
        zap::ztm_adc
    VENDOR_EXTENSIONS
)
endif()
