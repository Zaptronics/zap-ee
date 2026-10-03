// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/boards/ztpico2/ztm_psu_tec.h"

int ztpico2_ztm_psu_tec_init(ztpico2_ztm_psu_tec_t *binding,
                           ztm_psu_tec_t *module,
                           const ztpico2_ztm_psu_tec_config_t *config)
{
    if (!binding || !module || !config || !config->board ||
        config->board != ztpico2_active() || !config->board->ready ||
        (config->spi != spi0 && config->spi != spi1) ||
        config->cs_mux_id > 15u ||
        config->enable_pin < 0 || config->enable_pin > 15) {
        return -1;
    }

    const bus_spi_config_t spi_config = {
        .baud_hz = MCP4901_SPI_BAUD_HZ,
        .data_bits = 8u,
        .cpol = SPI_CPOL_0,
        .cpha = SPI_CPHA_0,
        .order = SPI_MSB_FIRST,
        .cs_setup_us = 0u,
        .cs_hold_us = 0u,
        .cs_high_us = 1u,
    };

    binding->connector = (ztpico2_connector_t){
        .board = config->board,
        .enable_pin = config->enable_pin,
        .input_pin = EX_NONE,
    };
    if (ztpico2_connector_prepare(&binding->connector) != 0) {
        return -2;
    }

    if (!ztmb_zap_spi_init(&binding->spi,
                           config->spi,
                           config->cs_mux_id,
                           config->enable_pin,
                           &spi_config,
                           ZAP_SPI_DEFAULT_TIMEOUT_US,
                           0u)) {
        return -3;
    }
    ztmb_zap_lock_init(&binding->lock, 500000u);

    const ztm_psu_tec_config_t module_config = {
        .spi = ztmb_zap_spi_device(&binding->spi),
        .time = ztmb_zap_time(),
        .lock = ztmb_zap_lock(&binding->lock),
        .control_context = &binding->connector,
        .enable_set = ztpico2_connector_enable_set,
        .enable_get = ztpico2_connector_enable_get,
        .ref_buffered = config->ref_buffered,
        .gain_1x = config->gain_1x,
    };
    return ztm_psu_tec_init(module, &module_config);
}
