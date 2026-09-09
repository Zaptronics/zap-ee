// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/framed_spi.h"

bool zap_spi_framed_device_valid(const zap_spi_framed_device_t *device)
{
    return device != NULL && device->ops != NULL &&
           device->ops->sequence_begin != NULL &&
           device->ops->frame != NULL &&
           device->ops->sequence_end != NULL;
}

int zap_spi_framed_sequence_begin(const zap_spi_framed_device_t *device)
{
    if (!zap_spi_framed_device_valid(device)) {
        return ZAP_SPI_E_BAD_ARG;
    }
    return device->ops->sequence_begin(device->context, device->timeout_us);
}

int zap_spi_framed_frame(const zap_spi_framed_device_t *device,
                         const uint8_t *tx,
                         uint8_t *rx,
                         size_t len)
{
    if (!zap_spi_framed_device_valid(device) || len == 0u ||
        (tx == NULL && rx == NULL)) {
        return ZAP_SPI_E_BAD_ARG;
    }
    return device->ops->frame(device->context, tx, rx, len,
                              device->read_fill, device->timeout_us);
}

int zap_spi_framed_sequence_end(const zap_spi_framed_device_t *device)
{
    if (!zap_spi_framed_device_valid(device)) {
        return ZAP_SPI_E_BAD_ARG;
    }
    return device->ops->sequence_end(device->context);
}
