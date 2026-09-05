# Porting ZapEE to another MCU, RTOS, or OS

ZapEE's public module and driver APIs are portable C11. Platform-specific code
stays in the consuming application (or in an optional platform adapter) and
fills the small transport/time/control contracts exposed by ZapEE.

## Start with the public headers

After CMake configure, or after running the optional `.\zap` helper, include the
module normally:

```c
#include <zaptronics/modules/ztm_pwm.h>
```

When the optional helper is used with the default layout, the same headers are
also directly browsable at `deps/zapee/include/zaptronics/...`.

## ZTM_PWM platform requirements

`ztm_pwm_config_t` currently needs:

- a `zap_i2c_bus_t`;
- a `zap_time_t` millisecond-delay callback;
- module power-set/power-get callbacks;
- an optional module IRQ-state callback; and
- an optional structured event callback.

The I2C `write_read()` callback must perform one atomic write -> repeated START ->
read transaction. `lock` and `unlock` are optional; a threaded application should
use them to protect a shared controller, while a deliberately single-threaded
bare-metal application may leave both NULL.

## ZTM_ADC platform requirements

`ztm_adc_config_t` needs:

- a `zap_spi_device_t` whose `begin()` / `transfer()` / `end()` operations keep the same device selected across a multi-part AD7124 transaction;
- a high-resolution `zap_time_t` providing microsecond delay and monotonic microsecond time;
- an optional `zap_lock_t` protecting complete high-level measurement/configuration sequences; and
- an optional structured event callback.

The SPI adapter owns bus/controller configuration and chip-select policy. The generic AD7124 driver assumes the supplied SPI endpoint is configured for the AD7124's required mode; it does not reconfigure a shared controller itself.

## Adapter skeleton

Projects using the optional helper can run:

```powershell
.\zap adapter my_platform
```

This creates an application-owned skeleton in `project.adapters_dir` (default
`adapters/my_platform`). It includes all current callback signatures and TODOs,
but deliberately returns `ZAP_I2C_E_UNSUPPORTED` until the integrator maps the
operations onto their SDK/RTOS.

The generated adapter belongs to the application because board-level choices
such as controller instance, bus frequency, GPIO ownership, module-enable wiring,
IRQ routing, mutex implementation, and timeout policy are application/platform
concerns rather than properties of a chip driver.

Before generating a custom adapter, check the built-in platform support. ZapEE
0.4.0 provides generic Pico SDK and native Zephyr I2C/SPI/lock/time adapters, plus convenience targets for both ZTM_PWM and ZTM_ADC.
See `PLATFORM_ADAPTERS.md` and `ZTM_PWM.md`.

## CMake inclusion

Add the generated adapter `.c` to your application target. ZapEE itself is not
modified. The public module remains linked as `zap::ztm_pwm`.

## Zephyr

ZapEE is also a native Zephyr external module. A Zephyr-specific adapter may use
Zephyr I2C, GPIO, timing, and mutex APIs internally; those native Zephyr types
must not leak into the portable ZapEE driver/module headers.
