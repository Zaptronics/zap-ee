# ZTPICO2 board support (ZapEE 0.7.0)

ZTPICO2 is a motherboard, not the cloud chamber application. Its optional
`zap::pico_ztpico2` component owns the Pico bus fabric, chip-select routing,
onboard PCAL6416/TLA2518 and connector controls. The independent `zap::pcal6416`
and `zap::tla2518` targets have portable transport APIs and no Pico or Hub types.

## Ownership

The application owns a persistent, initially zeroed `ztpico2_t` and calls
`ztpico2_init(&board, &config)` before initializing fitted modules. Only one
board can be active on an MCU: both physical SPI controllers share one decoder
and one fabric lock. Init/deinit require quiescent command and module users.
Failed initialization retains the active instance and its stage for diagnosis;
call deinit before retrying. Deinitialize modules before deinitializing the board.
Deinit lowers module enables but does not turn off the MCU's supply or remove SDK locks.

`ztpico2_connector_t` binds application-selected enable/input pins to portable
module callbacks. The application retains module instances, probe assignments,
the inventory/registry and structured-event handling. Low-level endpoint/adapter
names retain `bus_*`/`ztmb_zap_*` compatibility spelling in this first extraction.
These are Pico/ZTPICO2 backend APIs, not a second portable driver interface.

## Wiring carried forward

`include/zaptronics/boards/ztpico2/pins.h` is the single board wiring definition.
The migration preserves the existing source mapping and electrical defaults:

| Function | Pico GPIO / mapping |
| --- | --- |
| SPI0 MISO/SCK/MOSI | 0 / 2 / 3 |
| SPI1 MISO/SCK/MOSI | 12 / 10 / 11 |
| I2C0 SDA/SCL | 8 / 9 |
| I2C1 SDA/SCL | 18 / 19 |
| Expander IRQ | 20 |
| CS decoder address pins | 6, 5, 4 and 26, 27, 28 |
| Decoder idle addresses | 3 / 3 |
| Expander input mask / initial outputs | 0xA550 / 0x0000 |
| HV VSENSE | AIN1 |
| TEC1 / TEC2 / TEC3 / TEC4 VSENSE | AIN5 / AIN4 / AIN3 / AIN2 |

P1.7 retains its original input direction; this extraction does not reinterpret
the EX_3V3_EN name as an instruction to drive it. The supplied schematic contains
additional circuitry such as TCA9548 and DS28E07; no new support for those parts
is implied by moving the existing firmware. A full connectivity audit against
the KiCad sheet is not claimed. Hardware regression is skipped by user instruction.

## Transactions

The backend retains whole-fabric SPI ownership and per-endpoint mode/timing.
TLA2518 uses framed SPI so register command and readback hold one reservation
across both CS pulses. Mode changes are staged before releasing the sequence.
Board ADC reads lock the complete channel-select/read operation. The driver
returns ADC pin voltage; board-level reads apply the caller's divider ratio.
PCAL6416 accepts direction/output masks and optional operation locking; failed
writes leave cached output/direction bits unchanged.

## Optional services

- `zap::registry`: caller-owned entries, flags, event callback and yield callback.
- `zap::cli`: tokenization, numeric parsing and command-table lookup.
- `zap::log`: portable formatting with a submission callback.
- `zap::pico_log`: queued logs with an application-provided sink.
- `zap::pico_network`: WiFi CLI and HTTP with application credentials, banner,
  prompt, command callback and web assets. The application supplies its lwipopts.h
  include directory through `ZAPEE_PICO_NETWORK_CONFIG_DIR` before importing ZapEE.

Networking retains the existing one-session, firmware-lifetime service model,
blocking WiFi startup and synchronous command callbacks. A main-loop command
queue would change response/lifetime behavior and is a separate application
integration task. It is not silently introduced as part of this extraction.

## Handoff

This source change has not been built or tested. New PCAL6416, TLA2518 and registry
fixtures are included for the user's existing host-test workflow; none were run.
Hub 0.3.0 uses the sibling ZapEE checkout. Run the current Zap executable's
`sync --no-configure` from Hub before configuring/building; the old generated
lockfile/CMake integration intentionally remains untouched until Zap regenerates it.
Do not publish a manifest pin to v0.7.0 until that version actually exists remotely.

## TEC PSU binding (0.8.0)

The optional zap::pico_ztpico2_ztm_psu_tec component supplies caller-owned TEC
bindings. See [ZTPICO2_TEC_ADAPTER.md](ZTPICO2_TEC_ADAPTER.md) for ownership,
configuration and lifecycle details.

## PWM binding (0.9.0)

The optional zap::pico_ztpico2_ztm_pwm component supplies caller-owned PWM
bindings. See [ZTPICO2_PWM_ADAPTER.md](ZTPICO2_PWM_ADAPTER.md) for configuration
and lifecycle details.

## ADC binding (0.10.0)

The optional zap::pico_ztpico2_ztm_adc component supplies caller-owned ADC
bindings. See [ZTPICO2_ADC_ADAPTER.md](ZTPICO2_ADC_ADAPTER.md).

## Stepper binding (0.11.0)

The optional zap::pico_ztpico2_ztm_stepper component supplies caller-owned
stepper bindings. See [ZTPICO2_STEPPER_ADAPTER.md](ZTPICO2_STEPPER_ADAPTER.md).
