# SPDX-License-Identifier: Apache-2.0
zaptronics_add_component(
    NAME i2c
    SOURCES i2c.c
)

# SDK adapters are only defined when the consuming build has already imported
# the corresponding SDK. A normal host-only ZapEE build remains dependency free.
if(TARGET hardware_i2c AND TARGET pico_sync AND TARGET pico_time AND TARGET hardware_gpio)
    add_subdirectory(pico)
endif()

if(TARGET zephyr_interface)
    add_subdirectory(zephyr)
endif()
