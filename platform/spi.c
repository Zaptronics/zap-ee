// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/spi.h"

static uint32_t spi_timeout(const zap_spi_device_t *device)
{
    return device->timeout_us != 0u ? device->timeout_us : ZAP_SPI_DEFAULT_TIMEOUT_US;
}

bool zap_spi_device_valid(const zap_spi_device_t *device)
{
    return device != NULL && device->ops != NULL &&
           device->ops->begin != NULL &&
           device->ops->transfer != NULL &&
           device->ops->end != NULL;
}

int zap_spi_begin(const zap_spi_device_t *device)
{
    if (!zap_spi_device_valid(device)) {
        return ZAP_SPI_E_BAD_ARG;
    }
    return device->ops->begin(device->context, spi_timeout(device));
}

int zap_spi_transfer(const zap_spi_device_t *device,
                     const uint8_t *tx,
                     uint8_t *rx,
                     size_t len)
{
    if (!zap_spi_device_valid(device) || len == 0u || (tx == NULL && rx == NULL)) {
        return ZAP_SPI_E_BAD_ARG;
    }
    return device->ops->transfer(device->context,
                                 tx,
                                 rx,
                                 len,
                                 device->read_fill,
                                 spi_timeout(device));
}

int zap_spi_end(const zap_spi_device_t *device)
{
    if (!zap_spi_device_valid(device)) {
        return ZAP_SPI_E_BAD_ARG;
    }
    return device->ops->end(device->context);
}

static int spi_one_shot(const zap_spi_device_t *device,
                        const uint8_t *tx,
                        uint8_t *rx,
                        size_t len)
{
    int rc = zap_spi_begin(device);
    if (rc != ZAP_SPI_OK) {
        return rc;
    }

    int transfer_rc = zap_spi_transfer(device, tx, rx, len);
    int end_rc = zap_spi_end(device);
    return transfer_rc != ZAP_SPI_OK ? transfer_rc : end_rc;
}

int zap_spi_write(const zap_spi_device_t *device,
                  const uint8_t *tx,
                  size_t len)
{
    if (tx == NULL) {
        return ZAP_SPI_E_BAD_ARG;
    }
    return spi_one_shot(device, tx, NULL, len);
}

int zap_spi_read(const zap_spi_device_t *device,
                 uint8_t *rx,
                 size_t len)
{
    if (rx == NULL) {
        return ZAP_SPI_E_BAD_ARG;
    }
    return spi_one_shot(device, NULL, rx, len);
}

int zap_spi_exchange(const zap_spi_device_t *device,
                     const uint8_t *tx,
                     uint8_t *rx,
                     size_t len)
{
    if (tx == NULL || rx == NULL) {
        return ZAP_SPI_E_BAD_ARG;
    }
    return spi_one_shot(device, tx, rx, len);
}
