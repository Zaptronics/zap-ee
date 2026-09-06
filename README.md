# ZapEE

**Zaptronics Embedded Electronics Library**

Portable C drivers and module libraries for Zaptronics hardware.

This repository is deliberately independent of the Zaptronics Hub. The Hub is a
reference consumer; customers may use the same module libraries on other MCUs,
RTOSs, operating environments and architectures.

## Design goals

- portable C11 core;
- Apache-2.0 licensed source;
- strict ISO C11 for portable components; SDK adapters may enable vendor-required compiler extensions;
- standard CMake as the primary integration contract;
- native Zephyr external-module support;
- one source of truth for shared chip drivers;
- no mandatory package-manager executable or Zaptronics-specific installer;
- no Pico SDK, Zephyr, STM32 HAL or other vendor types in portable public APIs;
- explicit instances and optional locking rather than hidden global state;
- host-testable conversion/protocol logic.

## Repository layout

```text
include/zaptronics/       public headers
  drivers/                chip-driver public APIs (as extracted)
  modules/                Zaptronics-module public APIs (as extracted)
drivers/                  generic silicon implementations
modules/                  Zaptronics board/module implementations
platform/                 portable contracts + optional SDK adapters
cmake/                    component/package helpers
zephyr/                   native Zephyr module integration
tests/                    host-side tests
examples/                 consumer examples
```

A shared driver is one CMake target. If two modules require the same driver,
both link that target; its source is not copied into either module. When the
repository is consumed with FetchContent/add_subdirectory, component directories
are excluded from the parent project's default build and only linked targets are
built. A top-level ZapEE build builds all components for CI/install.

## Optional native `zap` project helper

For teams that prefer not to edit CMake for routine dependency/version changes,
Zaptronics provides the separate native [`Zaptronics/zap`](https://github.com/Zaptronics/zap)
command-line tool. It is a small Go-built executable for Windows, Linux and macOS;
no PowerShell, Python, Node.js, .NET, or other runtime is required.

```text
zap init
zap sync      # bare `zap` is equivalent
```

Projects keep a readable `zap.yml`. For Git dependencies that publish
`zap-package.yml`, the tool performs a depth-1 sparse checkout containing only
the requested components, their transitive component dependencies, and package
common paths. ZapEE publishes this metadata at its repository root.

The generated `cmake/zap_deps.cmake` remains ordinary CMake. If the sparse
`deps/zapee` checkout is absent, it falls back to normal shallow
`FetchContent(GIT_REPOSITORY/GIT_TAG)`, so installing `zap` is never required to
build a ZapEE project. See `docs/PROJECT_INTEGRATION.md`.

## Standard CMake: FetchContent

Pin a tested release tag:

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

The application names the Zaptronics module it wants. The module target owns its
transitive chip-driver dependencies.

### Local development override

CMake's native FetchContent override lets a firmware project use a local checkout
without editing its `CMakeLists.txt`:

```text
-DFETCHCONTENT_SOURCE_DIR_ZAPEE=/path/to/zap-ee
```

This will be the normal Hub-development workflow while a module library change is
being developed and tested before tagging a release.

## Standard CMake: Git submodule / source tree

Consumers that prohibit configure-time network access can vendor the repository
and use normal CMake:

```cmake
add_subdirectory(third_party/zap-ee EXCLUDE_FROM_ALL)
target_link_libraries(my_firmware PRIVATE zap::ztm_pwm)
```

## Standard CMake: installed package

For controlled build environments the repository can also be installed and
consumed with CMake package discovery:

```text
cmake -S . -B build
cmake --build build
cmake --install build --prefix /opt/zaptronics
```

Consumer:

```cmake
find_package(ZapEE CONFIG REQUIRED)
target_link_libraries(my_firmware PRIVATE zap::ztm_pwm)
```

## Zephyr

`zephyr/module.yml` makes this repository a native Zephyr external module. Add it
to a west workspace or set `ZEPHYR_MODULES`, then enable:

```ini
CONFIG_ZAPEE=y
```

Enable ZTM_PWM with `CONFIG_ZAPEE_ZTM_PWM=y` or ZTM_ADC with
`CONFIG_ZAPEE_ZTM_ADC=y`. Their standard Zephyr adapters are enabled by default
with the corresponding `*_ZEPHYR_ADAPTER` option; disable one only when the
application supplies its own platform adapter. Chip-driver dependencies remain
transitive.

See `zephyr/README.md`.

## Component naming

Public CMake targets use the `zap::` namespace:

```text
zap::core
zap::i2c
zap::spi
zap::sx1509
zap::pac195x
zap::emc2303
zap::ad7124
zap::thermocouple
zap::ztm_pwm
zap::ztm_adc
zap::pico
zap::pico_ztm_pwm
zap::pico_ztm_adc
zap::zephyr
zap::zephyr_ztm_pwm
zap::zephyr_ztm_adc
```

Source target names are internal implementation details.

## Component rule

Each extracted driver or module gets one directory and one target. Example:

```cmake
zaptronics_add_component(
    NAME sx1509
    SOURCES sx1509.c
)
```

A module expresses dependencies through target links:

```cmake
zaptronics_add_component(
    NAME ztm_pwm
    SOURCES ztm_pwm.c
    PUBLIC_DEPS
        zap::sx1509
        zap::pac195x
        zap::emc2303
)
```

CMake then owns dependency ordering and de-duplication.

## ZTM_PWM portability contract

`zap::ztm_pwm` is an explicit-instance board library. The caller owns a
`ztm_pwm_t` and supplies a `ztm_pwm_config_t` containing:

- a portable `zap_i2c_bus_t`;
- a portable millisecond delay callback;
- board power-enable callbacks;
- an optional IRQ-state callback; and
- an optional structured event callback for asynchronous module faults.

The portable source has no Pico SDK, Hub I/O-expander, console/logging, RTOS or
Zephyr dependency. See `docs/ZTM_PWM_MIGRATION.md` for the extraction and
reference-adapter details. I2C controller setup (including bus frequency) remains the
platform application's responsibility so a device library does not silently
reconfigure a shared industrial bus.

The `write_read` I2C adapter operation is explicitly one atomic
write/repeated-start/read transaction. Optional lock/unlock callbacks make a
shared bus safe in threaded environments without imposing an RTOS on bare-metal
applications.

## ZTM_ADC portability contract

`zap::ztm_adc` is also an explicit-instance board library. It uses the portable
`zap_spi_device_t` transaction contract, high-resolution `zap_time_t`, and an
optional `zap_lock_t` that protects complete multi-register measurement
sequences. The module emits structured reset/error/configuration events rather
than printing. AD7124 and thermocouple conversion logic are independent targets
and can be reused by other modules. See `docs/ZTM_ADC.md`.

## Supported platform adapters

For Pico SDK projects, link `zap::pico_ztm_pwm` or `zap::pico_ztm_adc`. The
generic `zap::pico` target provides I2C, SPI, locks and timing around
application-configured controllers. For Zephyr, enable the corresponding
`CONFIG_ZAPEE_*_ZEPHYR_ADAPTER` option and supply devices through
devicetree/application configuration.

These adapters are conveniences, not requirements. The Hub continues to use its
own adapter because its module enable/IRQ signals live behind a motherboard I/O
expander and its I2C controller is managed by `ztmb/bus`. See `docs/ZTM_PWM.md`
and `docs/PLATFORM_ADAPTERS.md`.

## Versioning

The repository uses Semantic Versioning tags. Firmware projects should pin a tag
(or commit for an intentionally unreleased integration), not a moving branch.

Version `0.x` is the extraction/stabilisation phase. Public driver/module APIs may
change while the portability boundary is being proven. A stable `1.0.0` should
only be tagged once the public transport/platform contract and first production
module APIs are considered supportable.

## Licensing

Licensed under the Apache License, Version 2.0. See `LICENSE`.
