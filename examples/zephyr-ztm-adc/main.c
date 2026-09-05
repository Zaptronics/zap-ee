#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zaptronics/modules/ztm_adc.h>
#include <zaptronics/platform/zephyr/ztm_adc.h>

/* Example expects a devicetree node alias named ztm_adc. */
static const struct spi_dt_spec adc_spi =
    SPI_DT_SPEC_GET(DT_ALIAS(ztm_adc),
                    SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA,
                    0);

static zap_zephyr_ztm_adc_adapter_t adapter;
static ztm_adc_t adc;

int example_ztm_adc_init(void)
{
    const zap_zephyr_ztm_adc_adapter_config_t adapter_config = {
        .spi = &adc_spi,
        .spi_timeout_us = 5000u,
        .operation_timeout_us = ZTM_ADC_OPERATION_TIMEOUT_US,
    };
    int rc = zap_zephyr_ztm_adc_adapter_init(&adapter, &adapter_config);
    if (rc != ZAP_SPI_OK) {
        return rc;
    }
    ztm_adc_config_t config = zap_zephyr_ztm_adc_make_config(&adapter);
    return ztm_adc_init(&adc, &config);
}
