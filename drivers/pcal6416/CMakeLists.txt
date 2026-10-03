# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME pcal6416
    SOURCES pcal6416.c
    PUBLIC_DEPS zap::i2c
)
