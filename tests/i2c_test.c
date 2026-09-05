// SPDX-License-Identifier: Apache-2.0
#include <zaptronics/platform/i2c.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    unsigned locks;
    unsigned unlocks;
    unsigned writes;
    unsigned write_reads;
    uint8_t address;
    uint32_t timeout_us;
    uint8_t write_data[8];
    size_t write_len;
} fake_i2c_t;

static int fake_lock(void *context, uint32_t timeout_us)
{
    fake_i2c_t *fake = (fake_i2c_t *)context;
    fake->locks++;
    fake->timeout_us = timeout_us;
    return 0;
}

static void fake_unlock(void *context)
{
    ((fake_i2c_t *)context)->unlocks++;
}

static int fake_write(void *context, uint8_t address, const uint8_t *data,
                      size_t len, uint32_t timeout_us)
{
    fake_i2c_t *fake = (fake_i2c_t *)context;
    fake->writes++;
    fake->address = address;
    fake->timeout_us = timeout_us;
    fake->write_len = len;
    if (len > sizeof fake->write_data) {
        return -1;
    }
    memcpy(fake->write_data, data, len);
    return 0;
}

static int fake_write_read(void *context, uint8_t address,
                           const uint8_t *write_data, size_t write_len,
                           uint8_t *read_data, size_t read_len,
                           uint32_t timeout_us)
{
    fake_i2c_t *fake = (fake_i2c_t *)context;
    fake->write_reads++;
    fake->address = address;
    fake->timeout_us = timeout_us;
    fake->write_len = write_len;
    if (write_len > sizeof fake->write_data) {
        return -1;
    }
    memcpy(fake->write_data, write_data, write_len);
    for (size_t i = 0; i < read_len; ++i) {
        read_data[i] = (uint8_t)(0xa0u + i);
    }
    return 0;
}

int main(void)
{
    fake_i2c_t fake = {0};
    const zap_i2c_bus_ops_t ops = {
        .write = fake_write,
        .write_read = fake_write_read,
        .probe = NULL,
        .lock = fake_lock,
        .unlock = fake_unlock,
    };
    const zap_i2c_bus_t bus = {
        .context = &fake,
        .ops = &ops,
        .timeout_us = 1234u,
    };
    zap_i2c_device_t dev = {0};
    if (zap_i2c_device_init(&dev, &bus, 0x28u) != ZAP_I2C_OK) {
        return 1;
    }

    const uint8_t payload[] = {0x11u, 0x22u};
    if (zap_i2c_write_reg(&dev, 0x55u, payload, sizeof payload) != ZAP_I2C_OK) {
        return 2;
    }
    if (fake.locks != 1u || fake.unlocks != 1u || fake.writes != 1u ||
        fake.address != 0x28u || fake.timeout_us != 1234u || fake.write_len != 3u ||
        fake.write_data[0] != 0x55u || fake.write_data[1] != 0x11u ||
        fake.write_data[2] != 0x22u) {
        return 3;
    }

    uint8_t readback[3] = {0};
    if (zap_i2c_read_reg(&dev, 0x66u, readback, sizeof readback) != ZAP_I2C_OK) {
        return 4;
    }
    if (fake.locks != 2u || fake.unlocks != 2u || fake.write_reads != 1u ||
        fake.write_len != 1u || fake.write_data[0] != 0x66u ||
        readback[0] != 0xa0u || readback[1] != 0xa1u || readback[2] != 0xa2u) {
        return 5;
    }

    return 0;
}
