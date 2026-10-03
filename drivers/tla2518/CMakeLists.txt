# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME tla2518
    SOURCES tla2518.c
    PUBLIC_DEPS zap::spi_framed
)
