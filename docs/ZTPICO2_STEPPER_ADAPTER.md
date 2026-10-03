# ZTPICO2 stepper adapter

Link `zap::pico_ztpico2_ztm_stepper` and include
`zaptronics/boards/ztpico2/ztm_stepper.h`. This optional component depends on
`zap::pico_ztpico2` and `zap::ztm_stepper`; the board target itself gains no module dependency.

The caller owns persistent `ztpico2_ztm_stepper_t` binding and `ztm_stepper_t`
module storage. Initialize with an active, ready board, spi0 or spi1, decoded
CS line 0..15, enable endpoint -1 (none) or 0..15, and optional event callback/context.
Each instance needs its own binding. Never copy, move or rebind live storage.
Board, binding, module and event context must outlive all operations.
Initialize/deinitialize only while module users are quiescent. The motherboard
backend still supports one active physical board per MCU.

Use `ztpico2_ztm_stepper_init`, then the ordinary `ztm_stepper_*` APIs, including
deinit. The power callback uses its binding context rather than global module
storage. Applications retain inventory, wiring assignments and diagnostic text.

Transport remains 1 MHz, SPI mode 3, 8-bit MSB-first, zero CS setup/hold,
1-us CS-high, ZAP_SPI_DEFAULT_TIMEOUT_US and zero read-fill. Framed SPI keeps
sequence ownership and the existing chain framing. Operation locking uses
ZTM_STEPPER_OPERATION_TIMEOUT_US (5000 us).

The adapter adds no power transition or delay. The unchanged portable initializer
switches off, waits 1 ms, switches on, waits 1 ms, resets, waits 1 ms and applies
defaults, including hard HiZ for the unused fourth motor. Deinit remains portable.

Invalid arguments return -1 before transport access; transport failure returns -2.
Otherwise the portable initializer's result is returned (negative codes overlap).
After failure, do not perform normal operations. If portable init was reached,
retain module/binding storage for deinit/recovery; earlier failures do not initialize
the module. No new recovery behavior is introduced.

Validation not run. Building, testing and hardware checks remain with the user.
