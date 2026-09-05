// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hardware/spi.h"
#include "pico/sync.h"
#include "zaptronics/platform/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t baud_hz;
    uint8_t data_bits;
    spi_cpol_t cpol;
    spi_cpha_t cpha;
    spi_order_t order;
    uint32_t cs_setup_us;
    uint32_t cs_hold_us;
    uint32_t cs_high_us;
} zap_pico_spi_config_t;

typedef struct {
    spi_inst_t *instance;
    mutex_t mutex;
    bool mutex_enabled;
} zap_pico_spi_bus_t;

typedef struct {
    zap_pico_spi_bus_t *bus;
    uint32_t cs_gpio;
    zap_pico_spi_config_t config;
    zap_spi_device_t device;
    bool active;
} zap_pico_spi_device_t;

/* The application owns spi_init() and SCK/MOSI/MISO pin mux configuration. */
int zap_pico_spi_bus_init(zap_pico_spi_bus_t *bus,
                          spi_inst_t *instance,
                          bool thread_safe);

/* The adapter owns only the direct GPIO chip-select pin supplied here. */
int zap_pico_spi_device_init(zap_pico_spi_device_t *adapter,
                             zap_pico_spi_bus_t *bus,
                             uint32_t cs_gpio,
                             const zap_pico_spi_config_t *config,
                             uint32_t timeout_us,
                             uint8_t read_fill);

const zap_spi_device_t *zap_pico_spi_device(const zap_pico_spi_device_t *adapter);

#ifdef __cplusplus
}
#endif
