# Optional `zap` project helper

`zap.ps1` plus the tiny `zap.cmd` launcher provide a friendly dependency workflow
for projects that use ZapEE but do not want to edit CMake for routine module or
version changes.

The helper is **optional**. ZapEE remains a normal CMake package and native
Zephyr module. Experienced users can use `FetchContent`, `add_subdirectory`,
`find_package(ZapEE)`, or a west manifest directly.

To bootstrap a Windows project, copy `zap.ps1` and `zap.cmd` to the project root
and run:

```powershell
.\zap new
.\zap
.\zap status
.\zap update zapee
```

`status` reports the resolved dependency checkout, Git HEAD, dirty/clean state
and whether it matches the manifest. `update <dependency>` queries the remote and
lists recent `vX.Y.Z` release tags without modifying the manifest; use
`zap version` to select one. Managed Git checkouts are never revision-switched
while dirty.

For Pico SDK projects, `zap new` places dependency loading after `pico_sdk_init()`
so SDK-specific ZapEE targets are discoverable, and defaults the first module example to `zap::pico_ztm_pwm`. Zephyr projects default the standard Zephyr ZTM_PWM adapter through Kconfig. Generic projects continue to use portable module targets and can generate an application-owned adapter skeleton. The generated skeleton includes the current I2C, SPI, operation-lock and timing contracts so it can be adapted for either ZTM_PWM or ZTM_ADC.

The project-root copy is intentional: the helper must exist before ZapEE itself
has been fetched. Its generated CMake is ordinary text and can be inspected,
committed, or replaced by hand-written CMake.

See `docs/PROJECT_INTEGRATION.md`, `docs/PLATFORM_ADAPTERS.md`, and
`docs/PORTING.md`.
