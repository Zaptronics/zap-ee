// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/drivers/pcal6416.h"
#include <string.h>

static bool write_reg(pcal6416a_t *dev, uint8_t reg, uint8_t value)
{
    return zap_i2c_write_reg(&dev->i2c, reg, &value, 1u) == ZAP_I2C_OK;
}
static bool read_reg(pcal6416a_t *dev, uint8_t reg, uint8_t *value)
{
    return zap_i2c_read_reg(&dev->i2c, reg, value, 1u) == ZAP_I2C_OK;
}
static bool acquire(pcal6416a_t *dev)
{
    return dev && zap_i2c_bus_valid(dev->i2c.bus) && zap_lock_acquire(dev->lock) == 0;
}
bool pcal6416a_init(pcal6416a_t *dev, const zap_i2c_bus_t *bus,
                    uint8_t address, uint16_t inputs, uint16_t outputs,
                    const zap_lock_t *lock)
{
    if (!dev || !zap_lock_valid(lock)) return false;
    memset(dev, 0, sizeof(*dev));
    if (zap_i2c_device_init(&dev->i2c, bus, address) != ZAP_I2C_OK) return false;
    dev->lock = lock;
    if (!acquire(dev)) return false;
    dev->out0 = (uint8_t)outputs;
    dev->out1 = (uint8_t)(outputs >> 8);
    dev->cfg0 = (uint8_t)inputs;
    dev->cfg1 = (uint8_t)(inputs >> 8);
    bool ok = write_reg(dev, PCAL6416A_REG_OUTCFG, 0u) &&
              write_reg(dev, PCAL6416A_REG_OUTPUT0, dev->out0) &&
              write_reg(dev, PCAL6416A_REG_OUTPUT1, dev->out1) &&
              write_reg(dev, PCAL6416A_REG_CONFIG0, dev->cfg0) &&
              write_reg(dev, PCAL6416A_REG_CONFIG1, dev->cfg1);
    zap_lock_release(dev->lock);
    if (!ok) dev->i2c.bus = NULL;
    return ok;
}
static bool update_pin(pcal6416a_t *dev, uint8_t pin, bool set, bool direction)
{
    if (pin > 15u || !acquire(dev)) return false;
    uint8_t *shadow = direction ? (pin < 8u ? &dev->cfg0 : &dev->cfg1)
                                : (pin < 8u ? &dev->out0 : &dev->out1);
    uint8_t reg = (uint8_t)((direction ? PCAL6416A_REG_CONFIG0 : PCAL6416A_REG_OUTPUT0) + (pin / 8u));
    uint8_t mask = (uint8_t)(1u << (pin & 7u));
    uint8_t value = set ? (uint8_t)(*shadow | mask) : (uint8_t)(*shadow & (uint8_t)~mask);
    bool ok = write_reg(dev, reg, value);
    if (ok) *shadow = value; /* Failed writes must not corrupt subsequent RMW. */
    zap_lock_release(dev->lock);
    return ok;
}
bool pcal6416a_write_pin(pcal6416a_t *dev, uint8_t pin, bool level)
{ return update_pin(dev, pin, level, false); }
bool pcal6416a_set_pin_direction(pcal6416a_t *dev, uint8_t pin, bool input)
{ return update_pin(dev, pin, input, true); }
bool pcal6416a_read_pin(pcal6416a_t *dev, uint8_t pin, bool *level)
{
    if (!level || pin > 15u || !acquire(dev)) return false;
    uint8_t value;
    bool ok = read_reg(dev, (uint8_t)(PCAL6416A_REG_INPUT0 + pin / 8u), &value);
    if (ok) *level = (value & (uint8_t)(1u << (pin & 7u))) != 0;
    zap_lock_release(dev->lock);
    return ok;
}
bool pcal6416a_read_all(pcal6416a_t *dev, uint16_t *value)
{
    if (!value || !acquire(dev)) return false;
    uint8_t low, high;
    bool ok = read_reg(dev, PCAL6416A_REG_INPUT0, &low) && read_reg(dev, PCAL6416A_REG_INPUT1, &high);
    if (ok) *value = (uint16_t)((uint16_t)low | ((uint16_t)high << 8));
    zap_lock_release(dev->lock);
    return ok;
}
