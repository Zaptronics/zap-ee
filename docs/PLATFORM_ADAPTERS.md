# Platform adapters

ZapEE separates portable device/module logic from SDK integration.

## Target layers

```text
portable modules/drivers
        |
        +-- zap::i2c             portable callback contract
        |
        +-- zap::pico            Pico SDK I2C/time implementation
        |     `-- zap::pico_ztm_pwm    Pico GPIO control for ZTM_PWM
        |
        `-- zap::zephyr          Zephyr I2C/time implementation
              `-- zap::zephyr_ztm_pwm  Zephyr GPIO control for ZTM_PWM
```

The generic platform targets are deliberately independent of any one Zaptronics
module so later modules can reuse them.

## Portable C versus SDK extensions

All portable ZapEE components compile as strict ISO C11 (`C_EXTENSIONS=NO`).
Platform adapter targets are still C11 code, but permit the compiler extensions
required by the imported vendor SDK. Vendor SDK types appear only in the
platform-specific headers under `zaptronics/platform/<sdk>/`.

## Pico SDK

`zap::pico` wraps an already-configured `i2c_inst_t`. It does not call
`i2c_init()`, select SDA/SCL pins, or set baud rate.

When thread safety is enabled it uses a Pico SDK `mutex_t`. Register reads use a
write with `nostop=true` followed by a read against one absolute deadline, so the
second phase starts with a repeated START. The Pico SDK documents this `nostop`
behaviour explicitly.

## Zephyr

`zap::zephyr` wraps an already-ready Zephyr I2C controller `struct device` and
uses `i2c_write_read()` for combined register reads. When enabled, a `k_mutex`
serializes ZapEE transactions.

The standard synchronous Zephyr I2C API has no portable per-call transfer
timeout argument, so transfer timeout policy belongs to the selected controller
driver/configuration.

## Custom platforms

A custom port can ignore all built-in SDK adapters and link `zap::ztm_pwm`
directly. Implement `zap_i2c_bus_ops_t`, `zap_time_t`, and the module control
callbacks described in `PORTING.md`. No fork of the module/driver source is
required.
