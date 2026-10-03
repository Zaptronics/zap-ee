# ZTPICO2 ADC adapter

Select zap::pico_ztpico2_ztm_adc in zap.yml or link it through ordinary CMake.
It depends on zap::pico_ztpico2 and zap::ztm_adc; neither acquires a reverse
dependency on this optional binding. Include zaptronics/boards/ztpico2/ztm_adc.h.

Allocate one persistent ztpico2_ztm_adc_t binding and ztm_adc_t module per ADC
module. Call ztpico2_ztm_adc_init with the active ready board, SPI controller,
decoded CS line, enable metadata and optional event callback/context.
Use distinct physical CS endpoints for multiple modules.

Timing is unchanged from the Hub adapter: 1 MHz, mode 3, 8-bit MSB-first,
zero CS setup/hold, one-microsecond CS-high, default SPI timeout and zero
read-fill. The operation lock uses ZTM_ADC_OPERATION_TIMEOUT_US. Existing
portable ADC initialization and conversion behavior remain unchanged.

The enable pin is endpoint metadata only. No power sequencing or connector
preparation is added; Hub supplies EX_NONE (-1), as before. Applications using
a powered connector must arrange its power separately.

Use normal ztm_adc_* operations and deinit. Binding/module addresses must remain
stable, and board and event context must outlive all operations. The temporary
config may go out of scope after init. Init, reinit and teardown require
quiescent users; never rebind a live module. Deinitialize modules before the
board. The board backend still supports one active motherboard per MCU.

Errors -1 and -2 indicate argument and transport failures respectively; otherwise
the portable initializer's result is returned. Codes overlap. Failed init does
not permit normal operations. If portable initialization was reached, retain
its state for deinit/recovery; earlier failures do not initialize a module.

Hub keeps its module instance, error messages, registry callbacks, bus diagnostic
accessor and chamber measurement assignments. This adapter has no Hub includes.
No build, test or hardware validation was performed for this migration.
