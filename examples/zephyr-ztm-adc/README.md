# Zephyr + ZTM_ADC

Define the ZTM_ADC SPI endpoint in devicetree (the sample uses alias `ztm_adc`),
add ZapEE as a Zephyr module, and enable the three options in `prj.conf`.

ZapEE copies the `spi_dt_spec` and adds `SPI_LOCK_ON | SPI_HOLD_ON_CS` internally
so the AD7124 command phase and data phase remain one chip-select transaction.
