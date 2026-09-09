# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME ad5282
    SOURCES ad5282.c
    PUBLIC_DEPS zap::i2c
)
