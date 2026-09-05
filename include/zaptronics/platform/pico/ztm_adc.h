// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>

#include "zaptronics/modules/ztm_adc.h"
#include "zaptronics/platform/pico/lock.h"
#include "zaptronics/platform/pico/spi.h"
#include "zaptronics/platform/pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    zap_pico_spi_bus_t *spi_bus;
    uint32_t cs_gpio;
    zap_pico_spi_config_t spi_config;
    uint32_t spi_timeout_us;
    uint32_t operation_timeout_us;
} zap_pico_ztm_adc_adapter_config_t;

typedef struct {
    zap_pico_spi_device_t spi;
    zap_pico_lock_t lock;
    zap_time_t time;
    bool initialized;
} zap_pico_ztm_adc_adapter_t;

int zap_pico_ztm_adc_adapter_init(
    zap_pico_ztm_adc_adapter_t *adapter,
    const zap_pico_ztm_adc_adapter_config_t *config);

ztm_adc_config_t zap_pico_ztm_adc_make_config(
    zap_pico_ztm_adc_adapter_t *adapter);

#ifdef __cplusplus
}
#endif
