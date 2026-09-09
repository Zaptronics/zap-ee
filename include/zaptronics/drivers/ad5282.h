// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zaptronics/platform/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AD5282 7-bit address: 0b01011 AD1 AD0 => 0x2C..0x2F. */
#define AD5282_ADDR(ad) ((uint8_t)((0x0Bu << 2) | ((ad) & 0x03u)))

typedef enum {
    AD5282_RDAC1 = 0,
    AD5282_RDAC2 = 1,
} ad5282_channel_t;

typedef enum {
    AD5282_OK = 0,
    AD5282_E_BAD_ARG = -1,
    AD5282_E_I2C = -2,
} ad5282_result_t;

typedef struct {
    zap_i2c_device_t i2c;
} ad5282_t;

/* Bind an instance to one already-configured I2C bus/address. No I/O is done. */
int ad5282_init(ad5282_t *dev, const zap_i2c_bus_t *bus, uint8_t address_7bit);
int ad5282_probe(ad5282_t *dev);

int ad5282_write_rdac(ad5282_t *dev,
                      ad5282_channel_t ch,
                      uint8_t code,
                      bool o1,
                      bool o2);
int ad5282_read_rdac(ad5282_t *dev,
                     ad5282_channel_t ch,
                     uint8_t *out_code);

int ad5282_series_set_ohms(ad5282_t *dev,
                           float rab_ohms,
                           float desired_ohms,
                           uint8_t *out_code_ch1,
                           uint8_t *out_code_ch2);

#ifdef __cplusplus
}
#endif
