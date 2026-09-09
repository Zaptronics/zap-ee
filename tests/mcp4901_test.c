#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zaptronics/drivers/mcp4901.h"

typedef struct {
    unsigned begin_count;
    unsigned end_count;
    uint8_t bytes[2];
    size_t len;
} fake_spi_t;

static int begin(void *context, uint32_t timeout_us)
{
    (void)timeout_us;
    ((fake_spi_t *)context)->begin_count++;
    return ZAP_SPI_OK;
}

static int transfer(void *context, const uint8_t *tx, uint8_t *rx,
                    size_t len, uint8_t read_fill, uint32_t timeout_us)
{
    (void)rx; (void)read_fill; (void)timeout_us;
    fake_spi_t *fake = context;
    assert(tx != NULL && len == 2u);
    memcpy(fake->bytes, tx, len);
    fake->len = len;
    return ZAP_SPI_OK;
}

static int end(void *context)
{
    ((fake_spi_t *)context)->end_count++;
    return ZAP_SPI_OK;
}

int main(void)
{
    fake_spi_t fake = {0};
    const zap_spi_device_ops_t ops = { begin, transfer, end };
    const zap_spi_device_t spi = {
        .context = &fake,
        .ops = &ops,
        .timeout_us = 5000u,
        .read_fill = 0u,
    };

    assert(mcp4901_make_frame(0x80u, true, true, true) == 0x7800u);
    assert(mcp4901_make_frame(0u, true, true, false) == 0x6000u);

    mcp4901_t dev = {0};
    assert(mcp4901_init(&dev, &spi, true, true) == MCP4901_OK);
    assert(mcp4901_set_raw(&dev, 0x80u) == MCP4901_OK);
    assert(fake.bytes[0] == 0x78u && fake.bytes[1] == 0x00u);
    assert(fake.begin_count == 1u && fake.end_count == 1u);

    assert(mcp4901_shutdown(&dev) == MCP4901_OK);
    assert(fake.bytes[0] == 0x60u && fake.bytes[1] == 0x00u);

    uint8_t code = 0u;
    assert(mcp4901_set_millivolts(&dev, 2500u, 1250u, &code) == MCP4901_OK);
    assert(code == 128u);
    assert(fake.bytes[0] == 0x78u && fake.bytes[1] == 0x00u);
    return 0;
}
