# ZTPICO2 TEC PSU adapter

Link zap::pico_ztpico2_ztm_psu_tec to use the ZTM_PSU_TEC module on a ZTPICO2.
It depends on zap::pico_ztpico2 and zap::ztm_psu_tec; the board and portable module
remain independently selectable. Ordinary CMake consumers link this target
directly after importing ZapEE. Zap users select the same component in zap.yml.

Include zaptronics/boards/ztpico2/ztm_psu_tec.h. Allocate one persistent
ztpico2_ztm_psu_tec_t binding and ztm_psu_tec_t module per physical TEC module.
Pass both to ztpico2_ztm_psu_tec_init with a config containing the active ready
board, SPI controller, decoded CS line, expander enable pin, and DAC reference
buffer/gain selections. Hub preserves true for both DAC selections.

The binding owns SPI endpoint, lock and connector state. The module retains
pointers into the binding, so neither may be copied or moved after init.
Multiple modules use distinct storage and physical endpoints. This does not
change the board backend's one-active-motherboard-per-MCU limitation.

Use the portable ztm_psu_tec_* operations, including deinit, after initialization.
Keep board and binding storage alive until users are quiescent and module
deinitialization has completed. Do not rebind an active module. The temporary
configuration structure may go out of scope after initialization.

The adapter preserves MCP4901 baud rate, mode 0, 8-bit MSB-first transfers,
one-microsecond CS-high time and a 500000-us operation lock timeout. Connector
preparation disables the enable output before module initialization. Hardware
failures are returned; software cannot guarantee output state after failed I/O.

Return values -1/-2/-3 identify argument, connector and transport setup failures,
respectively. The portable module initialization result is otherwise propagated;
negative values overlap and are not unique stage identifiers. A failed init does
not permit normal module operations. If portable module initialization was
reached, retain its state for module deinit/recovery. Early failures do not create
an initialized module. Recovery and any retry require quiescent users.

No Hub headers, inventory or control policy are used. This source migration was
not built or tested; hardware regression remains skipped.
