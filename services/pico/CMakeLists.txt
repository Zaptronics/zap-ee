zaptronics_add_component(
    NAME pico_log
    SOURCES log.c
    PUBLIC_DEPS zap::log pico_stdlib
    VENDOR_EXTENSIONS
)
