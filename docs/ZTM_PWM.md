# ZTM_PWM integration and API guide

ZTM_PWM is the reference ZapEE module. The board-level implementation is
portable C11 and depends only on ZapEE's I2C/time contracts plus application
callbacks for module enable, optional IRQ state, and optional events.

## Choose an integration path

| Environment | Recommended target | What you provide |
|---|---|---|
| Raspberry Pi Pico SDK | `zap::pico_ztm_pwm` | I2C peripheral/pins/baud and module GPIO numbers |
| Zephyr | `CONFIG_ZAPEE_ZTM_PWM_ZEPHYR_ADAPTER=y` | devicetree I2C bus + enable/IRQ GPIO specs |
| Other MCU/RTOS | `zap::ztm_pwm` | small adapter implementing the portable callbacks |
| Zaptronics Hub | `zap::ztm_pwm` | Hub-specific adapter through `ztmb/bus` and its I/O expander |

The public module include is always:

```c
#include <zaptronics/modules/ztm_pwm.h>
```

## Pico SDK quick start

The application configures the physical I2C controller. ZapEE deliberately does
not choose the bus frequency or pin mux because the controller may be shared.

```c
i2c_init(i2c0, 400000u);
gpio_set_function(4, GPIO_FUNC_I2C);
gpio_set_function(5, GPIO_FUNC_I2C);

zap_pico_ztm_pwm_adapter_t platform = {0};
zap_pico_ztm_pwm_adapter_config_t pcfg = {
    .i2c = i2c0,
    .i2c_timeout_us = 5000u,
    .thread_safe = true,
    .power_gpio = 10u,
    .power_active_high = true,
    .irq_gpio = 11,
    .irq_active_low = true,
    .irq_pull = ZAP_PICO_GPIO_PULL_UP,
};

zap_pico_ztm_pwm_adapter_init(&platform, &pcfg);
ztm_pwm_config_t cfg = zap_pico_ztm_pwm_make_config(&platform);

ztm_pwm_t pwm = {0};
ztm_pwm_init(&pwm, &cfg);
ztm_pwm_channel_set(&pwm, 1u, 25u);
```

Link only the top-level adapter target:

```cmake
target_link_libraries(app PRIVATE zap::pico_ztm_pwm)
```

That target brings in `zap::ztm_pwm`, `zap::pico`, and the SX1509/PAC195x/
EMC2303 drivers transitively.

## Zephyr quick start

Enable:

```ini
CONFIG_ZAPEE=y
CONFIG_ZAPEE_ZTM_PWM=y
CONFIG_ZAPEE_ZTM_PWM_ZEPHYR_ADAPTER=y
```

Use a Zephyr I2C controller device and `gpio_dt_spec` values for module enable
and IRQ. The adapter honors GPIO active-low/active-high flags. See
`examples/zephyr-ztm-pwm/` for a complete source example.

## Instance ownership

ZapEE never creates a global ZTM_PWM singleton. The application owns:

```c
ztm_pwm_t pwm;
```

Multiple modules can therefore coexist, provided each instance receives the
correct bus/control adapter.

## Initialization

`ztm_pwm_init()` starts from a safe state, configures the SX1509 PWM/GPIO device,
identifies/configures the three PAC195x current monitors and two EMC2303 fan
controllers, configures fan outputs, clears status, and records the last
initialization stage/error.

On failure inspect:

```c
pwm.last_init_stage
pwm.last_init_rc
ztm_pwm_init_stage_name(pwm.last_init_stage)
```

## Low-side PWM channels

There are twelve low-side channels, numbered 1..12. Duty cycle is integer
percent 0..100.

```c
ztm_pwm_channel_set(&pwm, 3u, 60u);
ztm_pwm_channel_stop(&pwm, 3u);
ztm_pwm_channel_set_all(&pwm, 20u);
ztm_pwm_stop_all(&pwm);
```

The physical SX1509 routing is board knowledge owned by `zap::ztm_pwm`; the
application never needs to know the SX1509 pin number.

## Current measurement

Each low-side channel has a board-defined PAC195x route. Use:

```c
int32_t current_uA;
ztm_pwm_read_current_uA(&pwm, 1u, true, &current_uA);
```

or retrieve the richer result:

```c
ztm_pwm_current_reading_t reading;
ztm_pwm_read_channel(&pwm, 1u, true, &reading);
```

The library also recognizes the PAC1952-2 salvage topology used by some board
revisions and marks coarse VBUS-derived measurements through
`reading.coarse_resolution`.

## Fan outputs and tachometers

Six fan channels are exposed as 1..6:

```c
ztm_pwm_fan_set(&pwm, 1u, 50u);
ztm_pwm_fan_set_target_rpm(&pwm, 1u, 1800u);
ztm_pwm_fan_get_rpm(&pwm, 1u, &rpm);
ztm_pwm_fan_stop(&pwm, 1u);
```

The module owns the mapping onto the two EMC2303 devices.

## General-purpose GPIO

Four module GPIOs are exposed as 1..4:

```c
ztm_pwm_gpio_configure_input(&pwm, 1u, ZTM_PWM_GPIO_PULL_UP);
ztm_pwm_gpio_read(&pwm, 1u, &level);

ztm_pwm_gpio_configure(&pwm, 2u, ZTM_PWM_GPIO_OUTPUT);
ztm_pwm_gpio_write(&pwm, 2u, true);
```

## IRQ and event handling

`ztm_pwm_poll()` is intentionally cheap when no IRQ is asserted. Applications
that connect the module IRQ should call it from normal thread/main-loop context.
It scans and clears latched SX1509/PAC195x/EMC2303 sources and emits structured
`ztm_pwm_event_t` callbacks when configured.

Events are data, not printf/log calls, so the application can route them to a
logger, CAN alarm, telemetry queue, PLC status register, or other system policy.

## Threading

ZapEE does not create threads. The portable I2C contract has optional lock/unlock
callbacks. The standard Pico and Zephyr adapters enable a mutex when
`.thread_safe = true`.

Do not call the mutex-enabled adapters from ISR context. A threaded application
should normally interact with ZTM_PWM from tasks/threads and use the physical IRQ
to wake or schedule that work rather than executing I2C transactions inside the
ISR.

## Timeouts

The Pico adapter uses one absolute Pico SDK deadline across a combined
write/repeated-START/read operation.

Zephyr's normal synchronous I2C API does not expose a per-transfer timeout, so
ZapEE's timeout controls mutex acquisition there; the selected Zephyr I2C driver
and controller configuration determine transfer timeout behaviour.

## I2C scan versus device probe

The generic platform adapters deliberately leave the optional raw-address
`probe` callback unset. A side-effect-free address probe is not equally portable
across SDKs/controllers.

`ztm_pwm_probe_devices()` still works because it identifies the six expected
chips using real register reads. `ztm_pwm_i2c_scan()` requires a platform adapter
that explicitly implements raw address probing and otherwise returns
`ZAP_I2C_E_UNSUPPORTED`.
