// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>
#include <zephyr/drivers/spi.h>
#include "zaptronics/modules/ztm_adc.h"
#include "zaptronics/platform/zephyr/lock.h"
#include "zaptronics/platform/zephyr/spi.h"
#include "zaptronics/platform/zephyr/time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const struct spi_dt_spec *spi;
    uint32_t spi_timeout_us;
    uint32_t operation_timeout_us;
} zap_zephyr_ztm_adc_adapter_config_t;

typedef struct {
    zap_zephyr_spi_t spi;
    zap_zephyr_lock_t lock;
    zap_time_t time;
    bool initialized;
} zap_zephyr_ztm_adc_adapter_t;

int zap_zephyr_ztm_adc_adapter_init(
    zap_zephyr_ztm_adc_adapter_t *adapter,
    const zap_zephyr_ztm_adc_adapter_config_t *config);

ztm_adc_config_t zap_zephyr_ztm_adc_make_config(
    zap_zephyr_ztm_adc_adapter_t *adapter);

#ifdef __cplusplus
}
#endif
