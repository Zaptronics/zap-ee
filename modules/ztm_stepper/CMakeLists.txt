# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME ztm_stepper
    SOURCES ztm_stepper.c
    PUBLIC_DEPS
        zap::l6470
        zap::spi_framed
)
