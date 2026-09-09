// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/drivers/ad5282.h"

static bool valid_channel(ad5282_channel_t ch)
{
    return ch == AD5282_RDAC1 || ch == AD5282_RDAC2;
}

int ad5282_init(ad5282_t *dev, const zap_i2c_bus_t *bus, uint8_t address_7bit)
{
    if (dev == NULL || zap_i2c_device_init(&dev->i2c, bus, address_7bit) != ZAP_I2C_OK) {
        return AD5282_E_BAD_ARG;
    }
    return AD5282_OK;
}

int ad5282_probe(ad5282_t *dev)
{
    if (dev == NULL) {
        return AD5282_E_BAD_ARG;
    }
    return zap_i2c_probe(&dev->i2c) == ZAP_I2C_OK ? AD5282_OK : AD5282_E_I2C;
}

/*
 * Instruction byte in normal write mode:
 *   [7] A/B: 0=RDAC1, 1=RDAC2
 *   [6] RS: midscale reset (kept 0 here)
 *   [5] SD: shutdown (kept 0; module-level enable remains authoritative)
 *   [4] O1
 *   [3] O2
 *   [2:0] don't care, written as 0
 */
static uint8_t make_instruction(ad5282_channel_t ch, bool o1, bool o2)
{
    uint8_t instruction = 0u;
    if (ch == AD5282_RDAC2) {
        instruction |= (uint8_t)(1u << 7);
    }
    if (o1) {
        instruction |= (uint8_t)(1u << 4);
    }
    if (o2) {
        instruction |= (uint8_t)(1u << 3);
    }
    return instruction;
}

int ad5282_write_rdac(ad5282_t *dev,
                      ad5282_channel_t ch,
                      uint8_t code,
                      bool o1,
                      bool o2)
{
    if (dev == NULL || !valid_channel(ch)) {
        return AD5282_E_BAD_ARG;
    }
    return zap_i2c_write_reg(&dev->i2c,
                             make_instruction(ch, o1, o2),
                             &code,
                             1u) == ZAP_I2C_OK
               ? AD5282_OK
               : AD5282_E_I2C;
}

int ad5282_read_rdac(ad5282_t *dev,
                     ad5282_channel_t ch,
                     uint8_t *out_code)
{
    if (dev == NULL || out_code == NULL || !valid_channel(ch)) {
        return AD5282_E_BAD_ARG;
    }
    return zap_i2c_read_reg(&dev->i2c,
                            make_instruction(ch, false, false),
                            out_code,
                            1u) == ZAP_I2C_OK
               ? AD5282_OK
               : AD5282_E_I2C;
}

int ad5282_series_set_ohms(ad5282_t *dev,
                           float rab_ohms,
                           float desired_ohms,
                           uint8_t *out_code_ch1,
                           uint8_t *out_code_ch2)
{
    if (dev == NULL || rab_ohms <= 0.0f) {
        return AD5282_E_BAD_ARG;
    }

    if (desired_ohms < 0.0f) {
        desired_ohms = 0.0f;
    }
    const float max_total = 2.0f * rab_ohms;
    if (desired_ohms > max_total) {
        desired_ohms = max_total;
    }

    const float total_codes_f = (desired_ohms / rab_ohms) * 256.0f;
    int total_codes = (int)(total_codes_f + 0.5f);
    /* Two 8-bit RDACs in series can represent at most 255 + 255 codes. */
    if (total_codes > 510) {
        total_codes = 510;
    }
    int code1 = total_codes / 2;
    int code2 = total_codes - code1;

    if (code1 < 0) {
        code1 = 0;
    }
    if (code2 < 0) {
        code2 = 0;
    }

    const int rc1 = ad5282_write_rdac(dev, AD5282_RDAC1, (uint8_t)code1, false, false);
    const int rc2 = ad5282_write_rdac(dev, AD5282_RDAC2, (uint8_t)code2, false, false);

    if (out_code_ch1 != NULL) {
        *out_code_ch1 = (uint8_t)code1;
    }
    if (out_code_ch2 != NULL) {
        *out_code_ch2 = (uint8_t)code2;
    }
    return rc1 == AD5282_OK && rc2 == AD5282_OK ? AD5282_OK : AD5282_E_I2C;
}
