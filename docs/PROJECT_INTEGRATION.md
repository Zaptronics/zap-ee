# Project integration

ZapEE does not require Hub headers or a Hub checkout. Applications provide
configuration, callbacks and assets. Zap is optional tooling.

## Direct CMake

After declaring the application project, import ZapEE from your chosen location:

```cmake
add_subdirectory(vendor/ZapEE EXCLUDE_FROM_ALL)
add_executable(my_firmware main.c)
target_link_libraries(my_firmware PRIVATE zap::pcal6416 zap::tla2518)
```

Link only the components you use. Portable drivers accept transport callbacks.
Pico components require the relevant Pico SDK targets to be initialized before
adding ZapEE. The optional board component, zap::pico_ztpico2, does not require
networking.

For zap::pico_network, supply the application-owned lwipopts.h directory before
importing ZapEE, directly or through generated integration:

```cmake
set(ZAPEE_PICO_NETWORK_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config"
    CACHE PATH "Directory containing the application's lwipopts.h")
add_subdirectory(vendor/ZapEE EXCLUDE_FROM_ALL)
```

The supplied directory must be absolute and exist. An empty setting adds no
include directory; an existing SDK/toolchain setup may supply it. This setting
is private to the network component. Applications compiling lwIP-dependent
sources must also supply their own include directories.

## Optional Zap integration

The sibling Zap repository is canonical for package-manager behavior. Consult
its README.md and docs/PACKAGE_MANIFEST.md for supported formats and commands.
Consumers use zap.yml; ZapEE publishes components in zap-package.yml. Select the
source location and components appropriate to your application. ZapEE assumes
no particular sibling directory layout.

Zap generates the lockfile and CMake integration; do not edit them manually.
Generated integration can enable Zap verification. This belongs to the generated
wrapper, not ZapEE's own CMake files. Direct consumption above does not import
that wrapper.

## Dependency boundary review

Source inspection found no ZapEE includes of Hub headers and no invocation of
the Zap executable in ZapEE's own CMake files. Credentials, web assets, module
inventory and chamber policy remain in Hub. Compatibility names such as
ztmb_zap_* in the board backend do not imply a dependency on Hub files.

This source review and configuration cleanup have not been built or tested.
Hardware regression remains skipped.
