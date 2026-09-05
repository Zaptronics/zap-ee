# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME ztm_pwm
    SOURCES ztm_pwm.c
    PUBLIC_DEPS
        zap::sx1509
        zap::pac195x
        zap::emc2303
)
