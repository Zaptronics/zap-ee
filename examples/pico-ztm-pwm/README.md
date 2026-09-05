# Pico SDK + ZTM_PWM example

This example shows the shortest supported Pico SDK path to a ZTM_PWM module.
It deliberately leaves I2C controller/pin/baud configuration in the application;
ZapEE does not silently reconfigure a shared bus.

The customer-facing dependency is `zap::pico_ztm_pwm`. It pulls in the portable
ZTM_PWM module, the Pico bus/time adapter, and all required chip drivers.

Change the example GPIO constants to suit the target board and connector wiring.
The module IRQ is optional: set `.irq_gpio = -1` if it is not connected.

The generic Pico adapter currently does not implement bus probing, because the
Pico SDK has no side-effect-free address-probe primitive in its normal I2C API.
Normal ZTM_PWM operation and `ztm_pwm_probe_devices()` use real register reads and
are unaffected; `ztm_pwm_i2c_scan()` returns `ZAP_I2C_E_UNSUPPORTED`.
