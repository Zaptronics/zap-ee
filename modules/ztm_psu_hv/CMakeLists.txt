# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME ztm_psu_hv
    SOURCES ztm_psu_hv.c
    PUBLIC_DEPS
        zap::ad5282
        zap::i2c
)
