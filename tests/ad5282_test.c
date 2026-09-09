#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zaptronics/drivers/ad5282.h"

typedef struct {
    uint8_t address;
    uint8_t rdac[2];
    uint8_t last_write[2];
    size_t last_write_len;
} fake_i2c_t;

static int fake_write(void *context, uint8_t address, const uint8_t *data,
                      size_t len, uint32_t timeout_us)
{
    (void)timeout_us;
    fake_i2c_t *fake = context;
    assert(fake != NULL && data != NULL && len == 2u);
    fake->address = address;
    memcpy(fake->last_write, data, len);
    fake->last_write_len = len;
    fake->rdac[(data[0] & 0x80u) != 0u ? 1u : 0u] = data[1];
    return 0;
}

static int fake_write_read(void *context, uint8_t address,
                           const uint8_t *write_data, size_t write_len,
                           uint8_t *read_data, size_t read_len,
                           uint32_t timeout_us)
{
    (void)timeout_us;
    fake_i2c_t *fake = context;
    assert(fake != NULL && write_data != NULL && write_len == 1u);
    assert(read_data != NULL && read_len == 1u);
    fake->address = address;
    read_data[0] = fake->rdac[(write_data[0] & 0x80u) != 0u ? 1u : 0u];
    return 0;
}

static int fake_probe(void *context, uint8_t address, uint32_t timeout_us)
{
    (void)context;
    (void)address;
    (void)timeout_us;
    return 0;
}

int main(void)
{
    fake_i2c_t fake = {0};
    const zap_i2c_bus_ops_t ops = {
        .write = fake_write,
        .write_read = fake_write_read,
        .probe = fake_probe,
    };
    const zap_i2c_bus_t bus = {
        .context = &fake,
        .ops = &ops,
        .timeout_us = 5000u,
    };

    ad5282_t dev = {0};
    assert(AD5282_ADDR(0u) == 0x2Cu);
    assert(AD5282_ADDR(3u) == 0x2Fu);
    assert(ad5282_init(&dev, &bus, AD5282_ADDR(2u)) == AD5282_OK);
    assert(ad5282_probe(&dev) == AD5282_OK);

    assert(ad5282_write_rdac(&dev, AD5282_RDAC2, 0x5Au, true, false) == AD5282_OK);
    assert(fake.address == 0x2Eu);
    assert(fake.last_write_len == 2u);
    assert(fake.last_write[0] == 0x90u);
    assert(fake.last_write[1] == 0x5Au);

    uint8_t code = 0u;
    assert(ad5282_read_rdac(&dev, AD5282_RDAC2, &code) == AD5282_OK);
    assert(code == 0x5Au);

    uint8_t c1 = 0u;
    uint8_t c2 = 0u;
    assert(ad5282_series_set_ohms(&dev, 10000.0f, 10000.0f, &c1, &c2) == AD5282_OK);
    assert(c1 == 128u && c2 == 128u);

    /* Clamp the full two-pot span to the representable 255 + 255 codes. */
    assert(ad5282_series_set_ohms(&dev, 10000.0f, 20000.0f, &c1, &c2) == AD5282_OK);
    assert(c1 == 255u && c2 == 255u);
    return 0;
}
