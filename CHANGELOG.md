# Changelog

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
