# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME sx1509
    SOURCES sx1509.c
    PUBLIC_DEPS zap::i2c
)
