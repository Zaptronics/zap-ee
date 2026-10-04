# Changelog

## 0.13.2

- Add emc2303_fan_set_min_drive and ztm_pwm_fan_set_min_drive to set the closed-loop minimum drive per fan.
- The EMC2303 default minimum drive is 40% (0x66), which stops slow RPM targets locking: a fan that needs about 20% duty spins up past its target instead.
- Defaults are unchanged; applications opt in. Update version fixture without running it.
- Validation not run: no builds, tests or rollback application.


## 0.13.1

- Echo accepted line-mode characters, erase on backspace/delete, and emit a newline before executing a line.
- Keep raw-key screen input un-echoed; preserve CRLF/CR-NUL framing.
- Update version fixture without running it.
- Validation not run: no builds, tests or rollback application.


## 0.13.0

- Add optional TCP console key, refresh and disconnect callbacks with raw-mode prompt suppression.
- Preserve Telnet decoding and entry CRLF framing; prevent HTTP from opening TCP screens.
- Expose output-idle backpressure and update the version fixture.
- Validation not run: no builds, tests or rollback application.


## 0.12.3

- Return failure from ztm_pwm_stop_all when the module is unavailable instead of skipping writes and reporting success.
- Preserve independent attempts to stop low-side and fan outputs when operational; propagate either failure.
- Document that zero-duty writes do not verify physical fan standstill; update the existing version fixture without running it.
- Validation not run: no builds, tests, or rollback application; hardware testing remains with the user.

## 0.12.2

- Subtract nominal 100 ohm protection resistance from external ZTM_ADC RTD1-RTD8 before reporting resistance and calculating temperature.
- Preserve CJC conversion, raw codes, and disconnected-input detection.
- Update the version fixture without running it.
- Validation not run: no builds, tests, or rollback application; hardware regression skipped.

## 0.12.1

- Correct SX1509 PWM duty to active-low sink time: 0% released, 100% sinking.
- With ZTM_PWM MCP1415 drivers, logical 0% now means load OFF, including startup and stop.
- Keep each SX1509 pin as an input while loading the latch and LED registers; enable output afterward.
- Clarify the public PWM duty contract and update the existing version fixture.
- Validation not run: no builds, tests, or rollback application; hardware retest remains with the user.

## 0.12.0

- Correct ZTM_PWM PAC1 address from 0x1F to 0x10 for R10 = 0 ohms to ground.
- Require POWER_SAFE feedback before PWM initialization, probing, and scanning.
- Configure ZTPICO2 TLA2518 GPIO0 (physical pin 15) as digital input under the ADC lock.
- Latch and report power-safe faults, disable enable, and block normal operations until reinitialization.
- Update the version fixture without executing it.
- Validation not run: builds and tests remain with the user.

## 0.11.0

- Add optional pico_ztpico2_ztm_stepper with caller-owned framed SPI, operation lock and power callback binding.
- Preserve stepper SPI timing, timeouts and portable initialization; retain application events and wiring in the consumer.
- Update the existing version fixture; no builds or tests were run.

## 0.10.0

- Add optional pico_ztpico2_ztm_adc with caller-owned SPI and operation-lock binding.
- Preserve ADC mode 3, 1 MHz timing, timeouts and portable initialization.
- Applications retain diagnostics and measurement assignments.
- No builds or tests were run; hardware regression remains skipped.

## 0.9.0

- Add optional pico_ztpico2_ztm_pwm with caller-owned I2C and connector bindings.
- Applications select IRQ wiring/polarity and event callbacks; no Hub dependency.
- Preserve Hub's bus defaults and connector preparation before module init.
- No builds or tests were run; hardware regression remains skipped.

## 0.8.0

- Add the optional pico_ztpico2_ztm_psu_tec component with caller-owned module bindings.
- Extract TEC SPI, lock and connector setup from Hub without application inventory or policy.
- Preserve the existing TEC timing and enable-off initialization sequence.
- Building and testing were not run; hardware regression remains skipped.

## 0.7.4

- Add standalone VS Code Pico SDK include configuration for ZapEE.
- Select application-generated SDK headers and compile commands using ZAPEE_PICO_BUILD_DIR, without a fixed Hub dependency.
- No build, tests, or editor validation were run.

## 0.7.3

- Make cs_mux.h self-contained by moving GPIO, board-pin and integer-width includes into cs_mux.c.
- Existing consumers include their board pin definitions independently.
- Building and testing were not run; validation remains with the user.

## 0.7.2

- Expose ZAPEE_PICO_NETWORK_CONFIG_DIR for application-owned lwIP configuration.
- Document direct CMake consumption and the independent library boundary.
- Building and testing remain with the user; validation was not run.

## 0.7.1 - Pico network SDK dependency

- Link pico_stdlib directly for the extracted network component's SDK headers and sleep_ms calls.
- Remove the unused pico/bootrom.h include.
- Update the version fixture; no builds or tests were run by the assistant.

## 0.7.0 - ZTPICO2 board and shared service extraction

- Extract portable PCAL6416 and TLA2518 drivers, with explicit transports.
- Add optional Pico ZTPICO2 motherboard support and connector controls.
- Extract registry, command parsing, logging and configurable Pico network services.
- Add user-run driver/registry fixtures; no builds or tests were run for this change.
- Hardware regression is skipped by user instruction.


## 0.6.0 - 2026-09-10

- Extract explicit-instance stepper, HV PSU and TEC PSU modules and L6470,
  AD5282 and MCP4901 drivers from Hub.
- Add sequence-owned framed SPI for L6470 chain traffic, Pico support, and host tests.
- Complete Zap package component metadata for the new modules and drivers.
- Validate the local integration with Hub; hardware regression remains pending.

## 0.5.0 - native Zap tooling and sparse package metadata

- Add root `zap-package.yml` metadata describing CMake components, transitive component dependencies, platform-adapter mappings, and sparse source paths.
- Make root/driver/module CMake discovery safe when unselected component directories are absent from a sparse checkout.
- Keep SDK-specific Pico/Zephyr module adapters conditional on the corresponding module source being present.
- Move the optional project helper out of ZapEE into the standalone cross-platform `Zaptronics/zap` Go application.
- Preserve normal shallow CMake `FetchContent` as the no-tool fallback; `zap` is an optional developer convenience rather than a build requirement.

## 0.4.1 - diagnostics, project-helper hardening and CI

- Add an operation-locked `ztm_adc_get_status()` snapshot API with device/configuration state, probe types, counters, last event/error and last measurement diagnostics.
- Preserve the last structured ZTM_ADC event for diagnostic snapshots.
- Refuse to overwrite dirty Git dependency checkouts and report missing refs with a clear `zap` error after fetch.
- Add `zap status` for resolved checkout/version/dirty-state diagnostics and `zap update <dependency>` to list remote release tags.
- Expand CI across GCC/Clang and Debug/Release with strict warnings, an installed-package consumer test, and a Windows `zap` helper integration fixture.
- Remove a dead SX1509 LED-register lookup table exposed by the stricter CI build.
- Document the ZTM_STEPPER/L6470 portability plan before extraction, including its distinct CS-per-byte daisy-chain transport requirement.

## 0.4.0 - portable SPI and ZTM_ADC

- Add the portable `zap::spi` device-transaction contract with explicit begin/transfer/end chip-select semantics.
- Add optional `zap_lock_t` operation-level locking and high-resolution microsecond timing callbacks.
- Extract the AD7124-8 driver and NIST thermocouple conversion library from the Hub.
- Extract ZTM_ADC as an explicit-instance `zap::ztm_adc` module with no Pico SDK, Hub logging, or global-state dependency.
- Add structured ZTM_ADC reset/error/configuration events.
- Add generic Pico SDK SPI/lock/time support plus `zap::pico_ztm_adc`.
- Add native Zephyr SPI/lock/time support plus `zap::zephyr_ztm_adc`.
- Add host tests for SPI transaction semantics and ZTM_ADC Pt1000/thermocouple conversion math.

## 0.3.0 - reference platform adapters and ZTM_PWM developer experience

- Add reusable Pico SDK I2C/time adapter target `zap::pico`.
- Add `zap::pico_ztm_pwm` for direct Pico GPIO enable/IRQ integration.
- Add native Zephyr I2C/time and ZTM_PWM GPIO adapters.
- Add complete Pico SDK and Zephyr ZTM_PWM examples.
- Add customer-facing ZTM_PWM API and platform-adapter documentation.
- Keep portable drivers/modules strict ISO C11 while allowing SDK adapter targets to use required vendor/compiler extensions.
- Document platform timeout/probe differences instead of hiding them behind false portability guarantees.

## 0.2.1 - optional project helper workflow

- Add the optional `zap` PowerShell project helper for YAML-managed dependencies.
- Document `zap.yml`, generated CMake glue, root-level dependency source checkouts, and the no-helper CMake path.
- Add project-porting guidance and adapter-generation workflow.
- Keep standard CMake/FetchContent and native Zephyr integration as first-class interfaces; the helper remains optional.

## 0.2.0 - ZTM_PWM portability migration

- Adopt Apache License 2.0 for the public source release.
- Require strict C11 for all reusable components.
- Add callback-based portable I2C and timing platform interfaces.
- Extract portable SX1509, PAC195x and EMC2303 drivers.
- Extract ZTM_PWM as an explicit-instance module with no Hub/Pico SDK dependency.
- Add optional structured module event callbacks for IRQ/fault reporting.
- Add native Zephyr `CONFIG_ZAPEE_ZTM_PWM` integration.
- Preserve transitive CMake dependencies so shared chip drivers compile once.

## 0.1.0 - repository foundation

- Standard CMake library/package foundation.
- `zap::` target namespace and reusable component helper.
- Native Zephyr external-module metadata and Kconfig entry point.
- FetchContent, add_subdirectory and installed `find_package` consumption paths.
- Host-side test and GitHub Actions baseline.
- No production chip drivers or Zaptronics modules extracted yet.
