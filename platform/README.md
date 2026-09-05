# Platform layer

The portable contract lives in `include/zaptronics/platform/` and contains no
MCU/RTOS types. SDK-specific adapters live in subdirectories and are only added
when the consuming build has already imported that SDK.

Current targets:

- `zap::i2c` - portable callback-based I2C transaction layer;
- `zap::pico` - Pico SDK I2C/time adapter;
- `zap::pico_ztm_pwm` - Pico GPIO control adapter for ZTM_PWM;
- `zap::zephyr` - Zephyr I2C/time adapter;
- `zap::zephyr_ztm_pwm` - Zephyr GPIO control adapter for ZTM_PWM.

Portable components are strict ISO C11. SDK adapter targets permit the vendor
compiler extensions required by their imported SDK, while keeping those types
and extensions outside portable driver/module APIs.

See `docs/PLATFORM_ADAPTERS.md`, `docs/PORTING.md`, and `docs/ZTM_PWM.md`.
