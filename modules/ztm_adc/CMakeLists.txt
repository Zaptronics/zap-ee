# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME ztm_adc
    SOURCES ztm_adc.c
    PUBLIC_DEPS
        zap::ad7124
        zap::thermocouple
)
if(NOT MSVC)
    target_link_libraries(zap_ztm_adc PUBLIC m)
endif()
