# Zephyr integration

The repository is a native Zephyr external module through `zephyr/module.yml`.
It can be discovered by `west`, or supplied directly using `ZEPHYR_MODULES`.

Example `west.yml` project entry:

```yaml
manifest:
  projects:
    - name: zap-ee
      url: https://github.com/Zaptronics/zap-ee
      revision: v0.3.0
      path: modules/lib/zap-ee
```

Then enable the library in `prj.conf`:

```ini
CONFIG_ZAPEE=y
CONFIG_ZAPEE_ZTM_PWM=y
```

`CONFIG_ZAPEE_ZTM_PWM` registers the same `zap::ztm_pwm` CMake component
used by non-Zephyr consumers. Its chip-driver dependencies are ordinary CMake
target dependencies and are included once automatically.

## Standard ZTM_PWM adapter

`CONFIG_ZAPEE_ZTM_PWM_ZEPHYR_ADAPTER=y` (default when ZTM_PWM is enabled)
registers `zap::zephyr_ztm_pwm`. It uses Zephyr I2C, `k_mutex`, `k_msleep`, and
`gpio_dt_spec` while keeping all Zephyr types outside the portable module API.

See `examples/zephyr-ztm-pwm/` and `docs/ZTM_PWM.md`.
