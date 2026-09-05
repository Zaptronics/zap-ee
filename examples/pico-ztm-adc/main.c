#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <zaptronics/modules/ztm_adc.h>
#include <zaptronics/platform/pico/ztm_adc.h>

static zap_pico_spi_bus_t spi_bus;
static zap_pico_ztm_adc_adapter_t adc_adapter;
static ztm_adc_t adc;

void example_ztm_adc_init(void)
{
    spi_init(spi1, 1000 * 1000);
    gpio_set_function(10, GPIO_FUNC_SPI); /* SCK */
    gpio_set_function(11, GPIO_FUNC_SPI); /* MOSI */
    gpio_set_function(12, GPIO_FUNC_SPI); /* MISO */

    zap_pico_spi_bus_init(&spi_bus, spi1, true);

    const zap_pico_ztm_adc_adapter_config_t adapter_config = {
        .spi_bus = &spi_bus,
        .cs_gpio = 13,
        .spi_config = {
            .baud_hz = 1000000u,
            .data_bits = 8u,
            .cpol = SPI_CPOL_1,
            .cpha = SPI_CPHA_1,
            .order = SPI_MSB_FIRST,
            .cs_high_us = 1u,
        },
        .spi_timeout_us = 5000u,
        .operation_timeout_us = ZTM_ADC_OPERATION_TIMEOUT_US,
    };
    zap_pico_ztm_adc_adapter_init(&adc_adapter, &adapter_config);

    ztm_adc_config_t config = zap_pico_ztm_adc_make_config(&adc_adapter);
    ztm_adc_init(&adc, &config);
}

bool example_read_rtd1(double *temperature_c)
{
    ztm_adc_rtd_sample_t sample;
    if (!temperature_c || !ztm_adc_read_rtd(&adc, 1u, &sample) ||
        sample.status != ZTM_ADC_SENSOR_OK) {
        return false;
    }
    *temperature_c = sample.temperature_c;
    return true;
}
