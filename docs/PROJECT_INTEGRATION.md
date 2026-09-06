# Project integration

ZapEE supports a friendly optional manifest workflow and completely standard
CMake/Zephyr integration. They use the same `zap::` component targets.

## 1. Native `zap` workflow

Install the cross-platform `Zaptronics/zap` executable from its GitHub Releases
page and put it on `PATH`. The tool is a self-contained Go binary; PowerShell,
Python, Node.js and .NET are not required.

From an existing CMake firmware project:

```text
zap init
zap sync
```

Bare `zap` is shorthand for `zap sync`. Interactive `zap init` detects Pico SDK,
Zephyr or generic CMake where possible, asks for the application target/board and
ZapEE repository, queries the newest stable Git tag, and lets the user select
ZapEE modules. The selected version is written explicitly into `zap.yml`; builds
do not float on a `latest` keyword.

Example:

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
    version: v0.5.0
    override_var: ZAPEE_SOURCE
    zephyr_module: true
    components:
      - zap::pico_ztm_pwm
```

Useful commands:

```text
zap init
zap sync
zap list
zap status
zap update zapee
zap version zapee v0.5.0
zap add zapee zap::ztm_adc
zap remove zapee zap::ztm_adc
zap adapter stm32
zap generate
zap check
```

### Sparse checkout

ZapEE's root `zap-package.yml` declares each component's source paths and
transitive component dependencies. `zap sync` fetches only the requested Git ref
with depth 1, reads that package manifest directly from the fetched commit, then
configures Git cone-mode sparse checkout before checking out the worktree.

For example, selecting only `zap::ztm_pwm` brings in the ZTM_PWM source, its
SX1509/PAC195x/EMC2303 drivers, common public headers/CMake, and the shared
transport/platform directory. ZTM_ADC, AD7124 and thermocouple implementation
sources are not checked out.

If a third-party dependency does not publish `zap-package.yml`, `zap` simply uses
a full **shallow** checkout for that dependency.

Managed checkouts are never revision-switched while dirty.

### No-tool fallback

Generated `cmake/zap_deps.cmake` first consumes `deps/<name>` when present. If it
is absent, the generated file falls back to ordinary shallow CMake
`FetchContent` using the URI/ref stored in `zap.yml`. Therefore a developer may
ignore `zap` completely and run normal CMake; only the sparse source-tree
optimisation and convenience commands are lost.

`deps/` should normally be ignored by the consuming project's Git repository.

### Pico SDK

A new Pico project can select `zap::pico_ztm_pwm` or `zap::pico_ztm_adc`. The
portable module and its chip drivers remain transitive. The application still
selects/configures the physical MCU bus and pins. Projects with motherboard-
specific routing, such as the Zaptronics Hub, may select `zap::ztm_pwm` /
`zap::ztm_adc` and provide their own adapter.

### Zephyr

For `environment: zephyr`, `zap sync` supplies dependencies marked
`zephyr_module: true` through `ZEPHYR_MODULES`, writes `cmake/zap_zephyr.conf`,
and invokes `west build --cmake-only`. ZapEE remains a native Zephyr external
module through `zephyr/module.yml`.

## 2. Standard CMake / Zephyr workflow

Nothing in ZapEE requires the `zap` executable. Equivalent CMake is simply:

```cmake
include(FetchContent)
FetchContent_Declare(
    zapee
    GIT_REPOSITORY https://github.com/Zaptronics/zap-ee.git
    GIT_TAG        v0.5.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(zapee)

target_link_libraries(my_firmware PRIVATE zap::ztm_pwm)
```

Zephyr users may instead add ZapEE to a west manifest or `ZEPHYR_MODULES` and
select the library through Kconfig.
