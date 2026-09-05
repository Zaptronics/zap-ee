# ZapEE

**Zaptronics Embedded Electronics Library**

Portable C drivers and module libraries for Zaptronics hardware.

This repository is deliberately independent of the Zaptronics Hub. The Hub is a
reference consumer; customers may use the same module libraries on other MCUs,
RTOSs, operating environments and architectures.

## Design goals

- portable C11 core;
- Apache-2.0 licensed source;
- strict C11 (`C_EXTENSIONS` disabled);
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
platform/                 platform-adapter contract and examples
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

## Optional `zap.yml` project helper

For teams that prefer not to edit CMake for routine dependency/version changes,
ZapEE also ships a small optional Windows helper under `tools/zap/`. Copy
`zap.ps1` and `zap.cmd` to the consuming project root, then use a human-readable
`zap.yml` manifest.

```powershell
.\zap new
.\zap
```

The helper writes ordinary `cmake/zap_deps.cmake`, keeps dependency source
checkouts in a configurable project folder such as `deps/`, and runs CMake
configure without compiling so IDE completion is immediately available. It can
also update dependency refs/components and generate an application-owned adapter
skeleton.

This layer is deliberately optional: standard CMake/FetchContent and native
Zephyr remain first-class interfaces. See `docs/PROJECT_INTEGRATION.md` and
`docs/PORTING.md`.

## Standard CMake: FetchContent

Pin a tested release tag:

```cmake
include(FetchContent)

FetchContent_Declare(
    zapee
    GIT_REPOSITORY https://github.com/Zaptronics/zap-ee.git
    GIT_TAG        v0.2.1
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

Enable ZTM_PWM with `CONFIG_ZAPEE_ZTM_PWM=y`. The module option pulls in
its SX1509, PAC195x and EMC2303 target dependencies automatically; the Zephyr
application does not list implementation chips itself.

See `zephyr/README.md`.

## Component naming

Public CMake targets use the `zap::` namespace:

```text
zap::core
zap::i2c
zap::sx1509
zap::pac195x
zap::emc2303
zap::ztm_pwm
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

## Versioning

The repository uses Semantic Versioning tags. Firmware projects should pin a tag
(or commit for an intentionally unreleased integration), not a moving branch.

Version `0.x` is the extraction/stabilisation phase. Public driver/module APIs may
change while the portability boundary is being proven. A stable `1.0.0` should
only be tagged once the public transport/platform contract and first production
module APIs are considered supportable.

## Licensing

Licensed under the Apache License, Version 2.0. See `LICENSE`.
