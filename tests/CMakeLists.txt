# SPDX-License-Identifier: Apache-2.0
add_executable(zap_version_test version_test.c)
target_link_libraries(zap_version_test PRIVATE zap::core)
add_test(NAME zap_version_test COMMAND zap_version_test)

add_executable(zap_i2c_test i2c_test.c)
target_link_libraries(zap_i2c_test PRIVATE zap::i2c)
add_test(NAME zap_i2c_test COMMAND zap_i2c_test)

add_executable(zap_spi_test spi_test.c)
target_link_libraries(zap_spi_test PRIVATE zap::spi)
add_test(NAME zap_spi_test COMMAND zap_spi_test)

add_executable(zap_ztm_pwm_mapping_test ztm_pwm_mapping_test.c)
target_link_libraries(zap_ztm_pwm_mapping_test PRIVATE zap::ztm_pwm)
add_test(NAME zap_ztm_pwm_mapping_test COMMAND zap_ztm_pwm_mapping_test)

add_executable(zap_ztm_adc_math_test ztm_adc_math_test.c)
target_link_libraries(zap_ztm_adc_math_test PRIVATE zap::ztm_adc)
add_test(NAME zap_ztm_adc_math_test COMMAND zap_ztm_adc_math_test)

add_executable(zap_ztm_adc_status_test ztm_adc_status_test.c)
target_link_libraries(zap_ztm_adc_status_test PRIVATE zap::ztm_adc)
add_test(NAME zap_ztm_adc_status_test COMMAND zap_ztm_adc_status_test)


add_executable(zap_framed_spi_test framed_spi_test.c)
target_link_libraries(zap_framed_spi_test PRIVATE zap::spi_framed)
add_test(NAME zap_framed_spi_test COMMAND zap_framed_spi_test)

add_executable(zap_l6470_test l6470_test.c)
target_link_libraries(zap_l6470_test PRIVATE zap::l6470)
add_test(NAME zap_l6470_test COMMAND zap_l6470_test)

add_executable(zap_ztm_stepper_frame_test ztm_stepper_frame_test.c)
target_link_libraries(zap_ztm_stepper_frame_test PRIVATE zap::ztm_stepper)
add_test(NAME zap_ztm_stepper_frame_test COMMAND zap_ztm_stepper_frame_test)

add_executable(zap_ztm_stepper_transport_test ztm_stepper_transport_test.c)
target_link_libraries(zap_ztm_stepper_transport_test PRIVATE zap::ztm_stepper)
add_test(NAME zap_ztm_stepper_transport_test COMMAND zap_ztm_stepper_transport_test)

add_executable(zap_ad5282_test ad5282_test.c)
target_link_libraries(zap_ad5282_test PRIVATE zap::ad5282)
add_test(NAME zap_ad5282_test COMMAND zap_ad5282_test)

add_executable(zap_mcp4901_test mcp4901_test.c)
target_link_libraries(zap_mcp4901_test PRIVATE zap::mcp4901)
add_test(NAME zap_mcp4901_test COMMAND zap_mcp4901_test)

add_executable(zap_ztm_psu_tec_test ztm_psu_tec_test.c)
target_link_libraries(zap_ztm_psu_tec_test PRIVATE zap::ztm_psu_tec)
add_test(NAME zap_ztm_psu_tec_test COMMAND zap_ztm_psu_tec_test)

add_executable(zap_ztm_psu_hv_test ztm_psu_hv_test.c)
target_link_libraries(zap_ztm_psu_hv_test PRIVATE zap::ztm_psu_hv)
add_test(NAME zap_ztm_psu_hv_test COMMAND zap_ztm_psu_hv_test)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    foreach(test_target
            zap_version_test
            zap_i2c_test
            zap_spi_test
            zap_ztm_pwm_mapping_test
            zap_ztm_adc_math_test
            zap_ztm_adc_status_test
            zap_framed_spi_test
            zap_l6470_test
            zap_ztm_stepper_frame_test
            zap_ztm_stepper_transport_test
            zap_ad5282_test
            zap_mcp4901_test
            zap_ztm_psu_tec_test
            zap_ztm_psu_hv_test)
        target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endforeach()
endif()

# New migration fixtures; provided for user-run host testing.
foreach(component pcal6416 tla2518 registry)
    add_executable(zap_${component}_test ${component}_test.c)
    target_link_libraries(zap_${component}_test PRIVATE zap::${component})
    add_test(NAME zap_${component}_test COMMAND zap_${component}_test)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(zap_${component}_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endforeach()
