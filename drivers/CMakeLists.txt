# SPDX-License-Identifier: Apache-2.0
foreach(_zapee_driver sx1509 pac195x emc2303 ad7124 thermocouple)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_zapee_driver}/CMakeLists.txt")
        add_subdirectory(${_zapee_driver})
    endif()
endforeach()
