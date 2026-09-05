# Zephyr integration

The repository is a native Zephyr external module through `zephyr/module.yml`.
It can be discovered by `west`, or supplied directly using `ZEPHYR_MODULES`.

Example `west.yml` project entry:

```yaml
manifest:
  projects:
    - name: zap-ee
      url: https://github.com/Zaptronics/zap-ee
      revision: v0.4.1
      path: modules/lib/zap-ee
```

Then enable the library and the modules required by the application in `prj.conf`:

```ini
CONFIG_ZAPEE=y
CONFIG_ZAPEE_ZTM_PWM=y
CONFIG_ZAPEE_ZTM_ADC=y
```

The module options register the same `zap::ztm_pwm` / `zap::ztm_adc` CMake
components used by non-Zephyr consumers. Their chip-driver dependencies are
ordinary CMake target dependencies and are included once automatically.

## Standard ZTM_PWM adapter

`CONFIG_ZAPEE_ZTM_PWM_ZEPHYR_ADAPTER=y` (default when ZTM_PWM is enabled)
registers `zap::zephyr_ztm_pwm`. It uses Zephyr I2C, `k_mutex`, `k_msleep`, and
`gpio_dt_spec` while keeping all Zephyr types outside the portable module API.

See `examples/zephyr-ztm-pwm/` and `docs/ZTM_PWM.md`.

## Standard ZTM_ADC adapter

`CONFIG_ZAPEE_ZTM_ADC_ZEPHYR_ADAPTER=y` (default when ZTM_ADC is enabled)
registers `zap::zephyr_ztm_adc`. It wraps a Zephyr `spi_dt_spec`, uses
`SPI_LOCK_ON | SPI_HOLD_ON_CS` for AD7124 multi-part register transactions,
provides `k_mutex` operation locking, and supplies microsecond timing from the
Zephyr kernel APIs.

See `examples/zephyr-ztm-adc/` and `docs/ZTM_ADC.md`.
