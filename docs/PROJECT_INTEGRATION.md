# Project integration

The sibling Zap repository is canonical for package-manager behavior. Its
README.md and docs/PACKAGE_MANIFEST.md describe the supported formats.

- A consumer uses `zap.yml` (current project schema 5).
- ZapEE publishes components in `zap-package.yml`. Schema 1 remains supported;
  schema 2 is required only when declaring dependencies on other packages.
- Zap generates `zap.lock` and CMake integration; do not edit them manually.

For local development, keep the repositories as siblings and use:

```yaml
schema: 5
project:
  environment: pico-sdk
  target: Hub
  board: pico2_w
  build_dir: build
  deps_dir: deps
dependencies:
  zapee:
    type: path
    uri: ../ZapEE
    components:
      - zap::ztm_pwm
      - zap::ztm_adc
      - zap::ztm_stepper
      - zap::ztm_psu_hv
      - zap::ztm_psu_tec
```

Run `zap sync --no-configure`, `zap verify --offline`, then `zap make --offline`.
Local path sources are developer-controlled, not an immutable release pin.
For released source, use a Git dependency with version v0.6.0 and the actual
repository URI, then regenerate with Zap. Hardware regression is still pending.

Git packages publish component paths and transitive component dependencies for
sparse checkout. CMake targets must express the same dependency graph. Hub uses
its own motherboard adapters; the standard Pico/Zephyr PWM and ADC adapters
remain available for projects with different board wiring.

Generated integration enables Zap verification by default. Direct CMake users
can explicitly select `-DZAP_VERIFY_DEPENDENCIES=OFF`; that disables verification.
Standalone FetchContent/add_subdirectory and installed-package consumption are
also supported by ZapEE itself.
