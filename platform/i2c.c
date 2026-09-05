// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/i2c.h"

#include <string.h>

static uint32_t timeout_for(const zap_i2c_bus_t *bus)
{
    return bus->timeout_us != 0u ? bus->timeout_us : ZAP_I2C_DEFAULT_TIMEOUT_US;
}

bool zap_i2c_bus_valid(const zap_i2c_bus_t *bus)
{
    if (bus == NULL || bus->ops == NULL ||
        bus->ops->write == NULL || bus->ops->write_read == NULL) {
        return false;
    }
    return (bus->ops->lock == NULL) == (bus->ops->unlock == NULL);
}

int zap_i2c_device_init(zap_i2c_device_t *device,
                        const zap_i2c_bus_t *bus,
                        uint8_t address_7bit)
{
    if (device == NULL || !zap_i2c_bus_valid(bus) || address_7bit > 0x7fu) {
        return ZAP_I2C_E_BAD_ARG;
    }
    device->bus = bus;
    device->address_7bit = address_7bit;
    return ZAP_I2C_OK;
}

static int lock_bus(const zap_i2c_bus_t *bus)
{
    if (bus->ops->lock == NULL) {
        return ZAP_I2C_OK;
    }
    return bus->ops->lock(bus->context, timeout_for(bus)) == 0
               ? ZAP_I2C_OK
               : ZAP_I2C_E_LOCK;
}

static void unlock_bus(const zap_i2c_bus_t *bus)
{
    if (bus->ops->unlock != NULL) {
        bus->ops->unlock(bus->context);
    }
}

int zap_i2c_send_byte(const zap_i2c_device_t *device, uint8_t value)
{
    if (device == NULL || !zap_i2c_bus_valid(device->bus)) {
        return ZAP_I2C_E_BAD_ARG;
    }
    const zap_i2c_bus_t *bus = device->bus;
    int rc = lock_bus(bus);
    if (rc != ZAP_I2C_OK) {
        return rc;
    }
    rc = bus->ops->write(bus->context,
                         device->address_7bit,
                         &value,
                         1u,
                         timeout_for(bus));
    unlock_bus(bus);
    return rc == 0 ? ZAP_I2C_OK : ZAP_I2C_E_IO;
}

int zap_i2c_probe(const zap_i2c_device_t *device)
{
    if (device == NULL || !zap_i2c_bus_valid(device->bus)) {
        return ZAP_I2C_E_BAD_ARG;
    }
    const zap_i2c_bus_t *bus = device->bus;
    if (bus->ops->probe == NULL) {
        return ZAP_I2C_E_UNSUPPORTED;
    }
    int rc = lock_bus(bus);
    if (rc != ZAP_I2C_OK) {
        return rc;
    }
    rc = bus->ops->probe(bus->context,
                         device->address_7bit,
                         timeout_for(bus));
    unlock_bus(bus);
    return rc == 0 ? ZAP_I2C_OK : ZAP_I2C_E_IO;
}

int zap_i2c_write_reg(const zap_i2c_device_t *device,
                      uint8_t reg,
                      const uint8_t *data,
                      size_t len)
{
    if (device == NULL || !zap_i2c_bus_valid(device->bus) ||
        (data == NULL && len != 0u) || len > (ZAP_I2C_MAX_WRITE_BYTES - 1u)) {
        return ZAP_I2C_E_BAD_ARG;
    }

    uint8_t frame[ZAP_I2C_MAX_WRITE_BYTES];
    frame[0] = reg;
    if (len != 0u) {
        memcpy(&frame[1], data, len);
    }

    const zap_i2c_bus_t *bus = device->bus;
    int rc = lock_bus(bus);
    if (rc != ZAP_I2C_OK) {
        return rc;
    }
    rc = bus->ops->write(bus->context,
                         device->address_7bit,
                         frame,
                         len + 1u,
                         timeout_for(bus));
    unlock_bus(bus);
    return rc == 0 ? ZAP_I2C_OK : ZAP_I2C_E_IO;
}

int zap_i2c_read_reg(const zap_i2c_device_t *device,
                     uint8_t reg,
                     uint8_t *data,
                     size_t len)
{
    if (device == NULL || !zap_i2c_bus_valid(device->bus) ||
        data == NULL || len == 0u) {
        return ZAP_I2C_E_BAD_ARG;
    }

    const zap_i2c_bus_t *bus = device->bus;
    int rc = lock_bus(bus);
    if (rc != ZAP_I2C_OK) {
        return rc;
    }
    rc = bus->ops->write_read(bus->context,
                              device->address_7bit,
                              &reg,
                              1u,
                              data,
                              len,
                              timeout_for(bus));
    unlock_bus(bus);
    return rc == 0 ? ZAP_I2C_OK : ZAP_I2C_E_IO;
}
