// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/drivers/spi.h>
#include "zaptronics/platform/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct spi_dt_spec spec;
    zap_spi_device_t device;
    bool active;
} zap_zephyr_spi_t;

/*
 * Wrap a devicetree SPI endpoint. ZapEE adds SPI_LOCK_ON/SPI_HOLD_ON_CS so
 * begin/transfer/end can preserve CS across multi-part chip transactions.
 */
int zap_zephyr_spi_init(zap_zephyr_spi_t *adapter,
                        const struct spi_dt_spec *spec,
                        uint32_t timeout_us,
                        uint8_t read_fill);
const zap_spi_device_t *zap_zephyr_spi_device(const zap_zephyr_spi_t *adapter);

#ifdef __cplusplus
}
#endif
