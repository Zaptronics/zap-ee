// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZAP_SPI_DEFAULT_TIMEOUT_US 5000u

typedef enum {
    ZAP_SPI_OK = 0,
    ZAP_SPI_E_BAD_ARG = -1,
    ZAP_SPI_E_IO = -2,
    ZAP_SPI_E_UNSUPPORTED = -3,
    ZAP_SPI_E_LOCK = -4,
    ZAP_SPI_E_STATE = -5,
} zap_spi_result_t;

/*
 * A ZapEE SPI device represents one chip-select endpoint, not merely a
 * controller peripheral. begin() must claim/configure the shared bus and
 * establish the device's CS transaction. transfer() clocks bytes while that
 * transaction remains active. end() deasserts CS and releases the bus.
 *
 * tx and rx may independently be NULL, but not both. When tx is NULL the
 * adapter transmits device->read_fill for each received byte. Callbacks must
 * not retain supplied buffers. timeout_us is a requested upper bound where
 * the platform can support one; adapters document any weaker SDK semantics.
 */
typedef struct {
    int (*begin)(void *context, uint32_t timeout_us);
    int (*transfer)(void *context,
                    const uint8_t *tx,
                    uint8_t *rx,
                    size_t len,
                    uint8_t read_fill,
                    uint32_t timeout_us);
    int (*end)(void *context);
} zap_spi_device_ops_t;

typedef struct {
    void *context;
    const zap_spi_device_ops_t *ops;
    uint32_t timeout_us;
    uint8_t read_fill;
} zap_spi_device_t;

bool zap_spi_device_valid(const zap_spi_device_t *device);
int zap_spi_begin(const zap_spi_device_t *device);
int zap_spi_transfer(const zap_spi_device_t *device,
                     const uint8_t *tx,
                     uint8_t *rx,
                     size_t len);
int zap_spi_end(const zap_spi_device_t *device);
int zap_spi_write(const zap_spi_device_t *device,
                  const uint8_t *tx,
                  size_t len);
int zap_spi_read(const zap_spi_device_t *device,
                 uint8_t *rx,
                 size_t len);
int zap_spi_exchange(const zap_spi_device_t *device,
                     const uint8_t *tx,
                     uint8_t *rx,
                     size_t len);

#ifdef __cplusplus
}
#endif
