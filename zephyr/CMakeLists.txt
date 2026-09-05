# SPDX-License-Identifier: Apache-2.0
if(NOT CONFIG_ZAPEE)
    return()
endif()

# Build the exact same CMake targets used by standalone/FetchContent consumers.
# Installation rules are irrelevant inside a Zephyr firmware build.
set(ZAPEE_INSTALL OFF CACHE BOOL "Generate install/export rules" FORCE)
set(ZAPEE_BUILD_TESTS OFF CACHE BOOL "Build ZapEE host-side tests" FORCE)

add_subdirectory(
    ${ZEPHYR_CURRENT_MODULE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/zap-ee
    EXCLUDE_FROM_ALL
)

# Make common headers available to a Zephyr application that explicitly enables
# the module. Future Kconfig-selected module targets will be registered with
# zaptronics_zephyr_enable_component(<name>).
target_link_libraries(zephyr_interface INTERFACE zap::core)

if(CONFIG_ZAPEE_ZTM_PWM)
    zaptronics_zephyr_enable_component(ztm_pwm)
endif()

if(CONFIG_ZAPEE_ZTM_PWM_ZEPHYR_ADAPTER)
    zaptronics_zephyr_enable_component(zephyr_ztm_pwm)
endif()

if(CONFIG_ZAPEE_ZTM_ADC)
    zaptronics_zephyr_enable_component(ztm_adc)
endif()

if(CONFIG_ZAPEE_ZTM_ADC_ZEPHYR_ADAPTER)
    zaptronics_zephyr_enable_component(zephyr_ztm_adc)
endif()
