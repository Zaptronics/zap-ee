# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20)

project(
    ZapEE
    VERSION 0.13.1
    DESCRIPTION "Zaptronics Embedded Electronics Library"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/ZapEEComponent.cmake)

# PROJECT_IS_TOP_LEVEL was added in CMake 3.21; keep compatibility with our
# declared CMake 3.20 minimum by using the equivalent source-directory test.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(ZAPEE_IS_TOP_LEVEL ON)
else()
    set(ZAPEE_IS_TOP_LEVEL OFF)
endif()

option(ZAPEE_BUILD_TESTS "Build ZapEE host-side tests" OFF)
option(ZAPEE_INSTALL "Generate install/export rules" ${ZAPEE_IS_TOP_LEVEL})

# Public common target. Every driver/module target links this target so common
# language requirements and public include paths stay consistent.
add_library(zap_core INTERFACE)
add_library(zap::core ALIAS zap_core)
set_target_properties(zap_core PROPERTIES EXPORT_NAME core)

target_compile_features(zap_core INTERFACE c_std_11)
target_include_directories(zap_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/include/zaptronics/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/include/zaptronics/version.h
    @ONLY
)

# When consumed as a dependency, keep all component directories out of the
# parent project's default ALL target: only linked modules/drivers are built.
# When this repository is the top-level project, build all components so CI and
# installation validate/package the complete released library set.
# Component directories may be absent in a zap-managed sparse checkout.
# Full source checkouts still see and build the complete component set.
foreach(_zapee_dir platform drivers modules services boards)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_zapee_dir}/CMakeLists.txt")
        if(ZAPEE_IS_TOP_LEVEL)
            add_subdirectory(${_zapee_dir})
        else()
            add_subdirectory(${_zapee_dir} EXCLUDE_FROM_ALL)
        endif()
    endif()
endforeach()

if(ZAPEE_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

if(ZAPEE_INSTALL)
    install(TARGETS zap_core EXPORT ZapEETargets)
    install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/zaptronics
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h")
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/generated/include/zaptronics/version.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zaptronics
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/ZapEEConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ZapEEConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZapEE
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ZapEEConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(EXPORT ZapEETargets
        FILE ZapEETargets.cmake
        NAMESPACE zap::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZapEE
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ZapEEConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ZapEEConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZapEE
    )
endif()
