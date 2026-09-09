#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zaptronics/modules/ztm_psu_tec.h"

typedef struct {
    bool enabled;
    unsigned enable_writes;
    uint8_t last_spi[2];
    unsigned spi_writes;
    uint32_t delayed_us;
    bool fail_spi;
} fake_t;

static int spi_begin(void *context, uint32_t timeout_us)
{
    (void)context; (void)timeout_us;
    return ZAP_SPI_OK;
}
static int spi_transfer(void *context, const uint8_t *tx, uint8_t *rx,
                        size_t len, uint8_t read_fill, uint32_t timeout_us)
{
    (void)rx; (void)read_fill; (void)timeout_us;
    fake_t *fake = context;
    assert(tx != NULL && len == 2u);
    memcpy(fake->last_spi, tx, 2u);
    fake->spi_writes++;
    if (fake->fail_spi) return ZAP_SPI_E_IO;
    return ZAP_SPI_OK;
}
static int spi_end(void *context) { (void)context; return ZAP_SPI_OK; }
static int enable_set(void *context, bool enabled)
{
    fake_t *fake = context;
    fake->enabled = enabled;
    fake->enable_writes++;
    return 0;
}
static int enable_get(void *context, bool *enabled)
{
    fake_t *fake = context;
    *enabled = fake->enabled;
    return 0;
}
static void delay_ms(void *context, uint32_t ms)
{
    ((fake_t *)context)->delayed_us += ms * 1000u;
}
static void delay_us(void *context, uint32_t us)
{
    ((fake_t *)context)->delayed_us += us;
}

int main(void)
{
    fake_t fake = {0};
    const zap_spi_device_ops_t spi_ops = { spi_begin, spi_transfer, spi_end };
    const zap_spi_device_t spi = { .context=&fake, .ops=&spi_ops, .timeout_us=5000u };
    const zap_time_t time = { .context=&fake, .delay_ms=delay_ms, .delay_us=delay_us };
    const ztm_psu_tec_config_t config = {
        .spi = &spi,
        .time = &time,
        .lock = NULL,
        .control_context = &fake,
        .enable_set = enable_set,
        .enable_get = enable_get,
        .ref_buffered = true,
        .gain_1x = true,
    };

    ztm_psu_tec_t dev = {0};
    assert(ztm_psu_tec_init(&dev, &config) == 0);
    assert(ztm_psu_tec_ready(&dev));
    assert(!fake.enabled);
    assert(fake.last_spi[0] == 0x60u && fake.last_spi[1] == 0x00u);

    assert(ztm_psu_tec_start(&dev, 100u, 250u));
    assert(fake.enabled);
    assert(fake.last_spi[0] == 0x76u && fake.last_spi[1] == 0x40u);
    assert(fake.delayed_us == 250u);

    ztm_psu_tec_status_t status = {0};
    assert(ztm_psu_tec_get_status(&dev, &status));
    assert(status.ready && status.enabled && status.enable_valid);
    assert(status.dac_active && status.dac_code == 100u);

    assert(ztm_psu_tec_stop(&dev));
    assert(!fake.enabled);
    assert(!dev.dac_active && dev.dac_code == 0u);
    assert(fake.last_spi[0] == 0x60u && fake.last_spi[1] == 0x00u);
    assert(ztm_psu_tec_start(&dev, 80u, 0u));
    fake.fail_spi = true;
    assert(!ztm_psu_tec_stop(&dev));
    assert(ztm_psu_tec_get_status(&dev, &status));
    assert(status.enable_valid && !status.enabled);
    assert(status.dac_active && status.dac_code == 80u);
    fake.fail_spi = false;
    assert(ztm_psu_tec_stop(&dev));
    return 0;
}
