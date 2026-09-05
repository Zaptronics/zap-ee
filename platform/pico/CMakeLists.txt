# SPDX-License-Identifier: Apache-2.0

# Pico SDK public headers expose SDK types, so this adapter is intentionally a
# platform-specific target. The portable ZapEE core remains strict ISO C11.
zaptronics_add_component(
    NAME pico
    SOURCES pico.c
    PUBLIC_DEPS
        zap::i2c
        hardware_i2c
        pico_sync
    PRIVATE_DEPS
        pico_time
    VENDOR_EXTENSIONS
)

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
