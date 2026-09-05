# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME emc2303
    SOURCES emc2303.c
    PUBLIC_DEPS zap::i2c
)
