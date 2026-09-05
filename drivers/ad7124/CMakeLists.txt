# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME ad7124
    SOURCES ad7124.c
    PUBLIC_DEPS zap::spi
)
