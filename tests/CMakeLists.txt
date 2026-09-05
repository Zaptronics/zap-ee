# SPDX-License-Identifier: Apache-2.0
add_executable(zap_version_test version_test.c)
target_link_libraries(zap_version_test PRIVATE zap::core)
add_test(NAME zap_version_test COMMAND zap_version_test)

add_executable(zap_i2c_test i2c_test.c)
target_link_libraries(zap_i2c_test PRIVATE zap::i2c)
add_test(NAME zap_i2c_test COMMAND zap_i2c_test)

add_executable(zap_ztm_pwm_mapping_test ztm_pwm_mapping_test.c)
target_link_libraries(zap_ztm_pwm_mapping_test PRIVATE zap::ztm_pwm)
add_test(NAME zap_ztm_pwm_mapping_test COMMAND zap_ztm_pwm_mapping_test)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    foreach(test_target zap_version_test zap_i2c_test zap_ztm_pwm_mapping_test)
        target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endforeach()
endif()
