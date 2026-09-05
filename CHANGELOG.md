# Changelog

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
