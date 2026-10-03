zaptronics_add_component(
    NAME pico_network
    SOURCES network.c http_server.c
    PUBLIC_DEPS zap::log pico_cyw43_arch_lwip_threadsafe_background pico_lwip pico_lwip_netif
    # Sources include pico/stdlib.h and use sleep_ms directly.
    PRIVATE_DEPS pico_stdlib
    VENDOR_EXTENSIONS
)
# Optional application-owned configuration; no consumer layout is assumed.
set(ZAPEE_PICO_NETWORK_CONFIG_DIR "" CACHE PATH
    "Absolute directory containing the application's lwipopts.h")
if(ZAPEE_PICO_NETWORK_CONFIG_DIR)
    if(NOT IS_ABSOLUTE "${ZAPEE_PICO_NETWORK_CONFIG_DIR}" OR
       NOT IS_DIRECTORY "${ZAPEE_PICO_NETWORK_CONFIG_DIR}")
        message(FATAL_ERROR
            "ZAPEE_PICO_NETWORK_CONFIG_DIR must be an existing absolute directory")
    endif()
    target_include_directories(zap_pico_network PRIVATE
        "$<BUILD_INTERFACE:${ZAPEE_PICO_NETWORK_CONFIG_DIR}>")
endif()
