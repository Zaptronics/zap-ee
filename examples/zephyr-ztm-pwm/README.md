# Zephyr + ZTM_PWM example

Add ZapEE as a Zephyr module (west manifest or `ZEPHYR_MODULES`) and enable the
options in `prj.conf`.

The example expects an application overlay to define the I2C controller and two
GPIO properties. A typical fragment is:

```dts
&i2c0 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;
};

/ {
    zephyr,user {
        ztm-pwm-enable-gpios = <&gpio0 10 GPIO_ACTIVE_HIGH>;
        ztm-pwm-irq-gpios = <&gpio0 11 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    };
};
```

Change controller/pins for the target board. `gpio_dt_spec` logical values honor
`GPIO_ACTIVE_LOW`, so no polarity logic is duplicated in the application.

The Zephyr synchronous I2C API does not expose a per-transfer timeout. ZapEE's
configured timeout is therefore used for adapter mutex acquisition; the I2C
transfer timeout itself follows the selected Zephyr controller driver/config.
