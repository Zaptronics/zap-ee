# Zephyr integration

The repository is a native Zephyr external module through `zephyr/module.yml`.
It can be discovered by `west`, or supplied directly using `ZEPHYR_MODULES`.

Example `west.yml` project entry:

```yaml
manifest:
  projects:
    - name: zap-ee
      url: https://github.com/Zaptronics/zap-ee
      revision: v0.2.0
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
