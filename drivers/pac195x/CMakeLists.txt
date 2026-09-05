# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME pac195x
    SOURCES pac195x.c
    PUBLIC_DEPS zap::i2c
)
