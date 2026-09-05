# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME thermocouple
    SOURCES thermocouple.c
)
if(NOT MSVC)
    target_link_libraries(zap_thermocouple PUBLIC m)
endif()
