# Minimal fragment for an existing Pico SDK application.
# Call pico_sdk_init() before ZapEE is made available.
target_link_libraries(my_firmware PRIVATE zap::pico_ztm_adc)
