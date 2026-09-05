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
0.3.0 provides `zap::pico` / `zap::pico_ztm_pwm` and native Zephyr adapters.
See `PLATFORM_ADAPTERS.md` and `ZTM_PWM.md`.

## CMake inclusion

Add the generated adapter `.c` to your application target. ZapEE itself is not
modified. The public module remains linked as `zap::ztm_pwm`.

## Zephyr

ZapEE is also a native Zephyr external module. A Zephyr-specific adapter may use
Zephyr I2C, GPIO, timing, and mutex APIs internally; those native Zephyr types
must not leak into the portable ZapEE driver/module headers.
