// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/boards/ztpico2/ztm_stepper.h"

static int power_set(void *context, bool enabled)
{
    ztpico2_ztm_stepper_t *binding = context;
    return bus_spi_en(&binding->spi.bus, enabled) ? 0 : -1;
}

int ztpico2_ztm_stepper_init(ztpico2_ztm_stepper_t *binding,
                       ztm_stepper_t *module,
                       const ztpico2_ztm_stepper_config_t *config)
{
    if (!binding || !module || !config || !config->board ||
        config->board != ztpico2_active() || !config->board->ready ||
        (config->spi != spi0 && config->spi != spi1) ||
        config->cs_mux_id > 15u ||
        config->enable_pin < -1 || config->enable_pin > 15) {
        return -1;
    }

    const bus_spi_config_t spi_config = {
        .baud_hz = 1000000u,
        .data_bits = 8u,
        .cpol = SPI_CPOL_1,
        .cpha = SPI_CPHA_1,
        .order = SPI_MSB_FIRST,
        .cs_setup_us = 0u,
        .cs_hold_us = 0u,
        .cs_high_us = 1u,
    };
    if (!ztmb_zap_framed_spi_init(&binding->spi, config->spi, config->cs_mux_id,
                           config->enable_pin, &spi_config,
                           ZAP_SPI_DEFAULT_TIMEOUT_US, 0u)) {
        return -2;
    }

    ztmb_zap_lock_init(&binding->lock, ZTM_STEPPER_OPERATION_TIMEOUT_US);
    const ztm_stepper_config_t module_config = {
        .spi = ztmb_zap_framed_spi_device(&binding->spi),
        .time = ztmb_zap_time(),
        .lock = ztmb_zap_lock(&binding->lock),
        .control_context = binding,
        .power_set = power_set,
        .event_context = config->event_context,
        .event = config->event,
    };
    return ztm_stepper_init(module, &module_config);
}
