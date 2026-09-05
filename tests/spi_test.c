// SPDX-License-Identifier: Apache-2.0
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zaptronics/platform/spi.h"

typedef struct {
    unsigned begin_count;
    unsigned transfer_count;
    unsigned end_count;
    bool active;
    uint8_t last_fill;
    uint8_t tx_seen[8];
    size_t tx_len;
} mock_spi_t;

static int mock_begin(void *context, uint32_t timeout_us)
{
    mock_spi_t *mock = context;
    if (mock == NULL || timeout_us != 1234u || mock->active) {
        return ZAP_SPI_E_IO;
    }
    mock->active = true;
    mock->begin_count++;
    return ZAP_SPI_OK;
}

static int mock_transfer(void *context,
                         const uint8_t *tx,
                         uint8_t *rx,
                         size_t len,
                         uint8_t read_fill,
                         uint32_t timeout_us)
{
    mock_spi_t *mock = context;
    if (mock == NULL || !mock->active || timeout_us != 1234u) {
        return ZAP_SPI_E_IO;
    }
    mock->transfer_count++;
    mock->last_fill = read_fill;
    if (tx != NULL) {
        if (len > sizeof(mock->tx_seen)) {
            return ZAP_SPI_E_IO;
        }
        memcpy(mock->tx_seen, tx, len);
        mock->tx_len = len;
    }
    if (rx != NULL) {
        for (size_t i = 0u; i < len; ++i) {
            rx[i] = tx != NULL ? tx[i] : read_fill;
        }
    }
    return ZAP_SPI_OK;
}

static int mock_end(void *context)
{
    mock_spi_t *mock = context;
    if (mock == NULL || !mock->active) {
        return ZAP_SPI_E_IO;
    }
    mock->active = false;
    mock->end_count++;
    return ZAP_SPI_OK;
}

int main(void)
{
    mock_spi_t mock = {0};
    const zap_spi_device_ops_t ops = {
        .begin = mock_begin,
        .transfer = mock_transfer,
        .end = mock_end,
    };
    const zap_spi_device_t device = {
        .context = &mock,
        .ops = &ops,
        .timeout_us = 1234u,
        .read_fill = 0xA5u,
    };

    const uint8_t write_data[] = {0x11u, 0x22u, 0x33u};
    if (zap_spi_write(&device, write_data, sizeof(write_data)) != ZAP_SPI_OK) {
        return 1;
    }
    if (mock.begin_count != 1u || mock.end_count != 1u ||
        mock.tx_len != sizeof(write_data) ||
        memcmp(mock.tx_seen, write_data, sizeof(write_data)) != 0) {
        return 2;
    }

    uint8_t read_data[3] = {0};
    if (zap_spi_read(&device, read_data, sizeof(read_data)) != ZAP_SPI_OK) {
        return 3;
    }
    if (read_data[0] != 0xA5u || read_data[1] != 0xA5u || read_data[2] != 0xA5u) {
        return 4;
    }

    if (zap_spi_begin(&device) != ZAP_SPI_OK) {
        return 5;
    }
    uint8_t response[2] = {0};
    const uint8_t command[2] = {0x9Au, 0xBCu};
    if (zap_spi_transfer(&device, command, response, sizeof(command)) != ZAP_SPI_OK) {
        return 6;
    }
    if (response[0] != command[0] || response[1] != command[1]) {
        return 7;
    }
    if (zap_spi_end(&device) != ZAP_SPI_OK) {
        return 8;
    }
    if (mock.begin_count != 3u || mock.end_count != 3u) {
        return 9;
    }
    return 0;
}
