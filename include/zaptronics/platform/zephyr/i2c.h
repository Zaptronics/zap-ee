// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "zaptronics/platform/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Zephyr adapter for a controller device that has already been configured. */
typedef struct {
    const struct device *device;
    zap_i2c_bus_t bus;
    struct k_mutex mutex;
    bool mutex_enabled;
} zap_zephyr_i2c_t;

/*
 * The portable timeout is used for mutex acquisition. Zephyr's synchronous I2C
 * API does not expose a per-transfer timeout, so transfer timeout behaviour is
 * determined by the selected Zephyr I2C controller driver/configuration.
 */
int zap_zephyr_i2c_init(zap_zephyr_i2c_t *adapter,
                        const struct device *device,
                        uint32_t timeout_us,
                        bool thread_safe);

const zap_i2c_bus_t *zap_zephyr_i2c_bus(const zap_zephyr_i2c_t *adapter);

#ifdef __cplusplus
}
#endif
