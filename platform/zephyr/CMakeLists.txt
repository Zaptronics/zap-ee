# SPDX-License-Identifier: Apache-2.0

zaptronics_add_component(
    NAME zephyr
    SOURCES zephyr.c
    PUBLIC_DEPS
        zap::i2c
        zap::spi
    VENDOR_EXTENSIONS
)

zaptronics_add_component(
    NAME zephyr_ztm_pwm
    SOURCES ztm_pwm.c
    PUBLIC_DEPS
        zap::zephyr
        zap::ztm_pwm
    VENDOR_EXTENSIONS
)

zaptronics_add_component(
    NAME zephyr_ztm_adc
    SOURCES ztm_adc.c
    PUBLIC_DEPS
        zap::zephyr
        zap::ztm_adc
    VENDOR_EXTENSIONS
)
