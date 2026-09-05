// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/pico/ztm_adc.h"

int zap_pico_ztm_adc_adapter_init(
    zap_pico_ztm_adc_adapter_t *adapter,
    const zap_pico_ztm_adc_adapter_config_t *config)
{
    if (adapter == NULL || config == NULL || config->spi_bus == NULL) {
        return ZAP_SPI_E_BAD_ARG;
    }

    adapter->initialized = false;
    int rc = zap_pico_spi_device_init(&adapter->spi,
                                      config->spi_bus,
                                      config->cs_gpio,
                                      &config->spi_config,
                                      config->spi_timeout_us,
                                      0u);
    if (rc != ZAP_SPI_OK) {
        return rc;
    }

    zap_pico_lock_init(&adapter->lock,
                       config->operation_timeout_us != 0u
                           ? config->operation_timeout_us
                           : ZTM_ADC_OPERATION_TIMEOUT_US);
    zap_pico_time_init(&adapter->time);
    adapter->initialized = true;
    return ZAP_SPI_OK;
}

ztm_adc_config_t zap_pico_ztm_adc_make_config(
    zap_pico_ztm_adc_adapter_t *adapter)
{
    ztm_adc_config_t config = {0};
    if (adapter == NULL || !adapter->initialized) {
        return config;
    }
    config.spi = zap_pico_spi_device(&adapter->spi);
    config.time = &adapter->time;
    config.lock = zap_pico_lock(&adapter->lock);
    return config;
}
