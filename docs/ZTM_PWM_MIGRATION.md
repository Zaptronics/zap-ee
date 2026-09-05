# ZTM_PWM migration notes

Version 0.2.0 is the first production module extraction from the Zaptronics Hub.

## Targets

```text
zap::ztm_pwm
  +-- zap::sx1509
  +-- zap::pac195x
  +-- zap::emc2303
        +-- zap::i2c
```

CMake owns this graph. A consumer links only `zap::ztm_pwm`; shared drivers are
not copied into modules and are compiled once per build graph.

## API change from the Hub-owned implementation

The old Hub module used a global `ztm_pwm_1` and Pico/Hub native types. The
portable API is explicit-instance:

```c
ztm_pwm_t pwm = {0};
ztm_pwm_config_t config = {
    .i2c = &application_i2c,
    .time = &application_time,
    .power_set = application_power_set,
    .power_get = application_power_get,
    .irq_read = application_irq_read,
    .event = application_event,
};

int rc = ztm_pwm_init(&pwm, &config);
```

All stateful operations take a `ztm_pwm_t *` or `const ztm_pwm_t *`. This allows
multiple instances and removes hidden global state.

## I2C contract

The platform provides `zap_i2c_bus_t`. `write_read` must be an atomic register
write followed by a repeated START and read while retaining bus ownership. The
optional lock/unlock pair lets RTOS/threaded applications serialize a shared
controller. Bare-metal applications can leave both callbacks NULL.

Bus speed belongs to the application/platform and is deliberately not part of a
chip driver's public configuration.

## Hub compatibility

Hub 0.2.21 is the reference adapter. It keeps the existing Hub I2C controller
locking/config cache, PCAL6416 connector control, module registry and logging,
but delegates board behavior to `zap::ztm_pwm`.

## Validation performed for 0.2.0

- strict `-std=c11 -Wall -Wextra -Wpedantic -Werror` syntax checks on all new
  portable production C sources;
- CMake configure/build of all components;
- host tests for package version, I2C transaction/locking semantics and ZTM_PWM
  static routing;
- CMake install/export followed by a separate `find_package()` consumer;
- FetchContent local-source consumer linking only `zap::ztm_pwm`;
- source scan confirming no Pico SDK or Hub types/includes remain in portable
  production directories.

A physical ZTM_PWM/Hub hardware regression build/test is still required after
applying the accompanying Hub patch.
