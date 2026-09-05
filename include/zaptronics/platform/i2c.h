// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZAP_I2C_DEFAULT_TIMEOUT_US 5000u
#define ZAP_I2C_MAX_WRITE_BYTES 64u

typedef enum {
    ZAP_I2C_OK = 0,
    ZAP_I2C_E_BAD_ARG = -1,
    ZAP_I2C_E_IO = -2,
    ZAP_I2C_E_UNSUPPORTED = -3,
    ZAP_I2C_E_LOCK = -4,
} zap_i2c_result_t;

/*
 * Bus callbacks return 0 on success and a negative value on failure.
 * write_read() must perform one atomic write/repeated-start/read transaction.
 * probe(), lock(), and unlock() are optional. If lock() is provided, unlock()
 * must also be provided. Callbacks must not retain the supplied buffers.
 *
 * timeout_us is a requested upper bound where the platform API can support one.
 * Some SDKs expose no portable per-transfer timeout; such adapters must document
 * their platform timeout policy rather than silently inventing semantics.
 */
typedef struct {
    int (*write)(void *context,
                 uint8_t address_7bit,
                 const uint8_t *data,
                 size_t len,
                 uint32_t timeout_us);
    int (*write_read)(void *context,
                      uint8_t address_7bit,
                      const uint8_t *write_data,
                      size_t write_len,
                      uint8_t *read_data,
                      size_t read_len,
                      uint32_t timeout_us);
    int (*probe)(void *context, uint8_t address_7bit, uint32_t timeout_us);
    int (*lock)(void *context, uint32_t timeout_us);
    void (*unlock)(void *context);
} zap_i2c_bus_ops_t;

typedef struct {
    void *context;
    const zap_i2c_bus_ops_t *ops;
    uint32_t timeout_us;
} zap_i2c_bus_t;

typedef struct {
    const zap_i2c_bus_t *bus;
    uint8_t address_7bit;
} zap_i2c_device_t;

bool zap_i2c_bus_valid(const zap_i2c_bus_t *bus);
int zap_i2c_device_init(zap_i2c_device_t *device,
                        const zap_i2c_bus_t *bus,
                        uint8_t address_7bit);
int zap_i2c_send_byte(const zap_i2c_device_t *device, uint8_t value);
int zap_i2c_probe(const zap_i2c_device_t *device);
int zap_i2c_write_reg(const zap_i2c_device_t *device,
                      uint8_t reg,
                      const uint8_t *data,
                      size_t len);
int zap_i2c_read_reg(const zap_i2c_device_t *device,
                     uint8_t reg,
                     uint8_t *data,
                     size_t len);

#ifdef __cplusplus
}
#endif
