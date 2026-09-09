# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME mcp4901
    SOURCES mcp4901.c
    PUBLIC_DEPS zap::spi
)
