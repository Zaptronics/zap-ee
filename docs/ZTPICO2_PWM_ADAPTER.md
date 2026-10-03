# ZTPICO2 PWM adapter

Link zap::pico_ztpico2_ztm_pwm and include
zaptronics/boards/ztpico2/ztm_pwm.h. The component depends on the independently
selectable zap::pico_ztpico2 and zap::ztm_pwm targets. It can be selected in
zap.yml or linked directly through ordinary CMake.

Allocate one persistent ztpico2_ztm_pwm_t binding and ztm_pwm_t module. Pass
them to ztpico2_ztm_pwm_init with an active ready board, I2C controller, enable
pin, IRQ pin/polarity and optional event callback/context. Set irq_pin to -1
when no IRQ input is connected; it supplies no IRQ callback to the module.

The adapter preserves BUS_I2C_DEFAULT_BAUD_HZ, BUS_I2C_DEFAULT_TIMEOUT_US and
connector preparation before module initialization. Preparation disables power,
configures the enable output and, if selected, the IRQ input. The portable
module retains its existing startup sequence. Hub selects EX_I2C_ALT_IO_1 as
an active-low input and retains its own event messages.

Bindings contain no singleton or application inventory. Each module needs its
own stable binding storage. ZTM_PWM chip addresses remain fixed, so multiple
modules cannot share the same electrically connected I2C address domain.
The board backend still supports one active physical motherboard per MCU.

Binding and module addresses must remain stable throughout use. Board and event
context must also outlive all operations. Configuration storage may go out of
scope after init. Use ordinary ztm_pwm_* APIs, including deinit. Init/reinit and
teardown require quiescent users; never rebind a live module.

Argument, connector and transport errors return -1, -2 and -3 respectively;
otherwise the portable initializer's result is propagated. Error codes overlap.
On failure do not perform normal operations. If portable init was reached,
retain its state for deinit/recovery; early failures do not initialize a module.
No new retry, locking or power-control policy is introduced.

No Hub includes or logging are used. No build, test or hardware validation was
performed for this migration.
