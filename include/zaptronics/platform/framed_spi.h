// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zaptronics/platform/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Framed SPI keeps ownership of a shared SPI fabric across a sequence while
 * pulsing chip-select around each complete frame. This is intended for
 * protocols such as L6470 daisy chains where every command byte is latched by
 * a separate CS pulse.
 */
typedef struct {
    int (*sequence_begin)(void *context, uint32_t timeout_us);
    int (*frame)(void *context,
                 const uint8_t *tx,
                 uint8_t *rx,
                 size_t len,
                 uint8_t read_fill,
                 uint32_t timeout_us);
    int (*sequence_end)(void *context);
} zap_spi_framed_device_ops_t;

typedef struct {
    void *context;
    const zap_spi_framed_device_ops_t *ops;
    uint32_t timeout_us;
    uint8_t read_fill;
} zap_spi_framed_device_t;

bool zap_spi_framed_device_valid(const zap_spi_framed_device_t *device);
int zap_spi_framed_sequence_begin(const zap_spi_framed_device_t *device);
int zap_spi_framed_frame(const zap_spi_framed_device_t *device,
                         const uint8_t *tx,
                         uint8_t *rx,
                         size_t len);
int zap_spi_framed_sequence_end(const zap_spi_framed_device_t *device);

#ifdef __cplusplus
}
#endif
