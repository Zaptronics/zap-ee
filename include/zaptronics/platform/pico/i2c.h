// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "pico/sync.h"
#include "zaptronics/platform/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pico SDK adapter for ZapEE's portable I2C contract.
 *
 * The application owns controller/pin/baud-rate setup. Call i2c_init() and
 * configure SDA/SCL before initialising this adapter. This prevents a library
 * from silently reconfiguring a shared industrial bus.
 */
typedef struct {
    i2c_inst_t *instance;
    zap_i2c_bus_t bus;
    mutex_t mutex;
    bool mutex_enabled;
} zap_pico_i2c_t;

/*
 * Initialise an adapter around an already-configured Pico SDK I2C controller.
 * When thread_safe is true, ZapEE transactions are serialised with a Pico SDK
 * mutex. The adapter must not be used from an IRQ context when mutex locking
 * is enabled.
 */
int zap_pico_i2c_init(zap_pico_i2c_t *adapter,
                      i2c_inst_t *instance,
                      uint32_t timeout_us,
                      bool thread_safe);

const zap_i2c_bus_t *zap_pico_i2c_bus(const zap_pico_i2c_t *adapter);

#ifdef __cplusplus
}
#endif
