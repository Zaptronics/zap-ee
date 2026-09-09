# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME ztm_psu_tec
    SOURCES ztm_psu_tec.c
    PUBLIC_DEPS
        zap::mcp4901
        zap::spi
)
