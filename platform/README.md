# Platform adaptation

Platform adapters connect portable Zaptronics drivers to an MCU SDK, RTOS or
bare-metal application. The first production transport contract was established
by the ZTM_PWM extraction in version 0.2.0.

## I2C

`zap_i2c_bus_t` contains caller-provided callbacks for write and atomic
write/repeated-start/read operations, plus optional probe and lock/unlock hooks.
Timeouts are explicit and bounded. The adapter owns controller configuration such
as bus frequency; chip drivers never reconfigure the MCU's shared I2C peripheral.

## Time

`zap_time_t` currently provides the millisecond delay operation needed during
device initialization. Additional timing primitives should only be added when a
real driver requires them.

## Design requirements

- no MCU/SDK/RTOS native types in reusable public driver headers;
- explicit caller-owned instances; no hidden global state;
- optional locking so bare-metal users pay no mandatory RTOS cost;
- bounded transfers and explicit timeout semantics;
- adapters may use Zephyr, Pico SDK, STM32 HAL, ESP-IDF, vendor SDKs or custom
  bare-metal implementations;
- host-side fake adapters remain possible for unit testing.

See `docs/PORTING.md` for the application adapter workflow and the optional `zap adapter` skeleton generator.
