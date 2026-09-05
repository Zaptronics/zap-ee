# Platform adapters

ZapEE separates portable device/module logic from SDK integration.

## Target layers

```text
portable modules/drivers
        |
        +-- zap::i2c / zap::spi / zap_lock_t / zap_time_t
        |
        +-- zap::pico            Pico SDK bus/lock/time implementation
        |     +-- zap::pico_ztm_pwm
        |     `-- zap::pico_ztm_adc
        |
        `-- zap::zephyr          Zephyr bus/lock/time implementation
              +-- zap::zephyr_ztm_pwm
              `-- zap::zephyr_ztm_adc
```

The generic platform targets are deliberately independent of any one Zaptronics
module so later modules can reuse them.

## Portable C versus SDK extensions

All portable ZapEE components compile as strict ISO C11 (`C_EXTENSIONS=NO`).
Platform adapter targets are still C11 code, but permit the compiler extensions
required by the imported vendor SDK. Vendor SDK types appear only in the
platform-specific headers under `zaptronics/platform/<sdk>/`.

## Pico SDK

`zap::pico` wraps application-owned I2C and SPI controllers. It does not call
`i2c_init()`/`spi_init()` or select bus pins. I2C frequency remains wholly
application-owned. SPI device adapters store per-device baud/mode and apply it
while the shared SPI bus mutex is held, because SPI mode is a device property.

When thread safety is enabled it uses a Pico SDK `mutex_t`. Register reads use a
write with `nostop=true` followed by a read against one absolute deadline, so the
second phase starts with a repeated START. The Pico SDK documents this `nostop`
behaviour explicitly.

## Zephyr

`zap::zephyr` wraps an already-ready Zephyr I2C controller `struct device` and
uses `i2c_write_read()` for combined register reads. When enabled, a `k_mutex`
serializes ZapEE transactions.

The standard synchronous Zephyr I2C/SPI APIs expose no common per-call transfer
timeout argument, so transfer timeout policy belongs to the selected controller
driver/configuration. The SPI adapter uses `SPI_LOCK_ON | SPI_HOLD_ON_CS` to
preserve multi-part device transactions until `zap_spi_end()`.

## Custom platforms

A custom port can ignore all built-in SDK adapters and link `zap::ztm_pwm`
directly. Implement the transport(s) required by the selected module (`zap_i2c_bus_ops_t`
and/or `zap_spi_device_ops_t`), `zap_time_t`, optional `zap_lock_t`, and any
board-control callbacks described in `PORTING.md`. No fork of the module/driver source is
required.
