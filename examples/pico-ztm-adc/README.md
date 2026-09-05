# Pico SDK + ZTM_ADC

This example shows the intended direct-GPIO Pico integration. The application
owns `spi_init()` and SPI pin muxing. `zap::pico_ztm_adc` supplies the shared SPI
bus/device adapter, high-resolution time provider, operation mutex, and portable
ZTM_ADC module.

Adjust the SPI pins and direct chip-select GPIO for your board. The Zaptronics
Hub does not use this direct-CS adapter because its chip-selects are motherboard
muxed; it uses its own adapter while linking the same `zap::ztm_adc` source.
