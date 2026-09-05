# Project integration

ZapEE supports two equally valid integration styles.

## 1. Friendly `zap.yml` workflow

Projects may keep a human-readable dependency manifest in the project root:

```yaml
schema: 1

project:
  environment: pico-sdk
  target: my_firmware
  board: pico2_w
  build_dir: build
  deps_dir: deps
  adapters_dir: adapters

dependencies:
  zapee:
    type: git
    uri: https://github.com/Zaptronics/zap-ee.git
    version: v0.4.0
    override_var: ZAPEE_SOURCE
    zephyr_module: true
    components:
      - zap::ztm_pwm
```

The optional `zap` helper converts this into `cmake/zap_deps.cmake`. The generated
file uses normal CMake `FetchContent` and normal target links; it is not a new
build system.

Typical commands:

```powershell
.\zap new
.\zap                     # sync deps + CMake configure only
.\zap list
.\zap version zapee v0.4.0
.\zap add zapee zap::ztm_pwm
.\zap adapter stm32
```

`deps_dir` controls where dependency **source checkouts** live. By default this is
`./deps`, giving IDE users a stable, browsable tree such as:

```text
deps/
  zapee/
    include/zaptronics/...
    drivers/...
    modules/...
```

Generated dependency build objects remain in the normal CMake build tree. This
keeps architecture/toolchain-specific build state out of `deps/`.

The helper understands a deliberately small YAML subset (two-space indentation,
scalar keys, and simple lists) so it has no Python, PyYAML, PowerShell-module, or
package-manager dependency. Dependency types currently supported by generated
CMake are `git`, `url`, and `path`.


### Built-in platform adapters

The manifest can select a platform-specific convenience target instead of the
portable module target. For a new Pico SDK project using direct MCU GPIOs for
module enable/IRQ:

```yaml
    components:
      - zap::pico_ztm_pwm
```

`zap::pico_ztm_pwm` pulls `zap::ztm_pwm` and all chip drivers transitively.
`zap::pico_ztm_adc` does the same for ZTM_ADC and adds the generic Pico SPI and
operation-lock adapters. The application still chooses/configures its MCU bus
instances and pins. The Zaptronics Hub intentionally keeps `zap::ztm_pwm` because its
control signals and bus locking are motherboard-managed.

For Zephyr, use the portable component/module selection plus Kconfig:

```yaml
    components:
      - zap::ztm_pwm
    zephyr_config:
      - CONFIG_ZAPEE=y
      - CONFIG_ZAPEE_ZTM_PWM=y
      - CONFIG_ZAPEE_ZTM_PWM_ZEPHYR_ADAPTER=y
```

### Pico SDK

For `environment: pico-sdk`, `.\zap` synchronises dependencies then calls the
project's `build.ps1 -ConfigureOnly`. This fetches dependencies and refreshes
`compile_commands.json`/IDE metadata without compiling firmware.

### Zephyr

For `environment: zephyr`, the helper synchronises Git dependencies before the
Zephyr configure step, passes dependencies marked `zephyr_module: true` through
`ZEPHYR_MODULES`, writes `cmake/zap_zephyr.conf` from any `zephyr_config` list,
and invokes `west build --cmake-only`.

Example Zephyr additions:

```yaml
    zephyr_module: true
    zephyr_config:
      - CONFIG_ZAPEE=y
      - CONFIG_ZAPEE_ZTM_PWM=y
```

## 2. Standard CMake / Zephyr workflow

Nothing in ZapEE requires the helper. The equivalent CMake remains:

```cmake
include(FetchContent)
FetchContent_Declare(
    zapee
    GIT_REPOSITORY https://github.com/Zaptronics/zap-ee.git
    GIT_TAG        v0.4.0
)
FetchContent_MakeAvailable(zapee)

target_link_libraries(my_firmware PRIVATE zap::ztm_pwm)
```

Zephyr users may instead add ZapEE to a west manifest or `ZEPHYR_MODULES` and
select the library through Kconfig.

## Generated-file policy

A project may commit both `zap.yml` and `cmake/zap_deps.cmake`. Doing so means a
plain CMake build still works on a machine that never runs `zap`; the helper is
only needed when the manifest changes or when a user wants the convenience
commands.

`deps/` should normally be ignored by Git. It is a local dependency-source cache,
not project source.
