# SPDX-License-Identifier: Apache-2.0
include_guard(GLOBAL)
include(CMakeParseArguments)

# Define one reusable Zaptronics component.
#
# Example:
#   zaptronics_add_component(
#       NAME sx1509
#       SOURCES sx1509.c
#   )
#
# Public headers live under <repo>/include/zaptronics/... and are installed by
# the top-level project. Dependencies are CMake target names, normally zap::...
function(zaptronics_add_component)
    set(options)
    set(one_value_args NAME)
    set(multi_value_args SOURCES PUBLIC_DEPS PRIVATE_DEPS COMPILE_DEFINITIONS)
    cmake_parse_arguments(ZC "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT ZC_NAME)
        message(FATAL_ERROR "zaptronics_add_component requires NAME")
    endif()
    if(NOT ZC_SOURCES)
        message(FATAL_ERROR "zaptronics_add_component(${ZC_NAME}) requires SOURCES")
    endif()

    set(target "zap_${ZC_NAME}")
    if(TARGET ${target})
        message(FATAL_ERROR "Zaptronics component '${ZC_NAME}' is already defined")
    endif()

    add_library(${target} STATIC ${ZC_SOURCES})
    add_library(zap::${ZC_NAME} ALIAS ${target})
    set_target_properties(${target} PROPERTIES EXPORT_NAME ${ZC_NAME})

    target_compile_features(${target} PUBLIC c_std_11)
    set_target_properties(${target} PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
    )
    target_link_libraries(${target} PUBLIC zap_core ${ZC_PUBLIC_DEPS})
    if(ZC_PRIVATE_DEPS)
        target_link_libraries(${target} PRIVATE ${ZC_PRIVATE_DEPS})
    endif()
    if(ZC_COMPILE_DEFINITIONS)
        target_compile_definitions(${target} PRIVATE ${ZC_COMPILE_DEFINITIONS})
    endif()

    # When the standalone project is embedded inside a Zephyr build, linking
    # zephyr_interface propagates Zephyr's compiler flags/defines to this
    # component and all of its transitive Zaptronics dependencies.
    if(TARGET zephyr_interface)
        target_link_libraries(${target} PUBLIC zephyr_interface)
    endif()

    if(ZAPEE_INSTALL)
        install(TARGETS ${target}
            EXPORT ZapEETargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endfunction()

# Add an already-defined Zaptronics component to Zephyr's final link. Only
# user-selected top-level components need to be enabled; ordinary CMake target
# dependencies pull shared chip drivers in exactly once.
function(zaptronics_zephyr_enable_component name)
    if(NOT TARGET zephyr_interface OR NOT COMMAND zephyr_append_cmake_library)
        message(FATAL_ERROR "zaptronics_zephyr_enable_component() requires a Zephyr build")
    endif()

    set(target "zap_${name}")
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Unknown Zaptronics component '${name}'")
    endif()

    get_property(already_enabled TARGET ${target} PROPERTY ZAPTRONICS_ZEPHYR_ENABLED)
    if(NOT already_enabled)
        zephyr_append_cmake_library(${target})
        set_property(TARGET ${target} PROPERTY ZAPTRONICS_ZEPHYR_ENABLED TRUE)
    endif()
endfunction()
