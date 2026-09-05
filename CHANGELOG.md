# Changelog

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
