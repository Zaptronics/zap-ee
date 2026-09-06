# SPDX-License-Identifier: Apache-2.0
foreach(_zapee_module ztm_pwm ztm_adc)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_zapee_module}/CMakeLists.txt")
        add_subdirectory(${_zapee_module})
    endif()
endforeach()
