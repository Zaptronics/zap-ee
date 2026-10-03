zaptronics_add_component(
    NAME pico_ztpico2
    SOURCES pico/board.c pico/bus.c pico/cs_mux.c pico/zap_adapter.c pico/connector.c
    PUBLIC_DEPS zap::pcal6416 zap::tla2518 zap::i2c zap::spi zap::spi_framed
                hardware_spi hardware_i2c hardware_gpio pico_sync pico_stdlib
    VENDOR_EXTENSIONS
)

# Board-only consumers must not acquire a dependency on the TEC module.
if(TARGET zap::ztm_psu_tec)
    zaptronics_add_component(
        NAME pico_ztpico2_ztm_psu_tec
        SOURCES pico/ztm_psu_tec.c
        PUBLIC_DEPS zap::pico_ztpico2 zap::ztm_psu_tec
        VENDOR_EXTENSIONS
    )
endif()

# PWM binding is optional; the board target stays independent of modules.
if(TARGET zap::ztm_pwm)
    zaptronics_add_component(
        NAME pico_ztpico2_ztm_pwm
        SOURCES pico/ztm_pwm.c
        PUBLIC_DEPS zap::pico_ztpico2 zap::ztm_pwm
        VENDOR_EXTENSIONS
    )
endif()

# ADC binding is optional; the board target stays independent of modules.
if(TARGET zap::ztm_adc)
    zaptronics_add_component(
        NAME pico_ztpico2_ztm_adc
        SOURCES pico/ztm_adc.c
        PUBLIC_DEPS zap::pico_ztpico2 zap::ztm_adc
        VENDOR_EXTENSIONS
    )
endif()

# Stepper binding is optional; the board target stays independent of modules.
if(TARGET zap::ztm_stepper)
    zaptronics_add_component(
        NAME pico_ztpico2_ztm_stepper
        SOURCES pico/ztm_stepper.c
        PUBLIC_DEPS zap::pico_ztpico2 zap::ztm_stepper
        VENDOR_EXTENSIONS
    )
endif()
