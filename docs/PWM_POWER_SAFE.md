# PWM POWER_SAFE bring-up — ZapEE 0.12.0 / Hub 0.3.8

PAC1/U2 uses address 0x10: R10 straps ADDRSEL to ground. PAC2 and PAC3
remain 0x11 and 0x12. SX1509 remains 0x71.

POWER_SAFE holds SX1509 reset low until the module supplies are ready.
The ZTPICO2 adapter configures TLA2518 physical pin 15 (AIN0/GPIO0) as a
digital input, preserving other channels. Configuration and reads share the
motherboard ADC operation lock with analog conversions.

The portable PWM configuration now requires power_safe_read (active high,
0 return for a valid read) using control_context. The ZTPICO2 adapter supplies
it. Generic Pico/Zephyr configuration factories require the caller to add it;
absence fails closed instead of falling back to a fixed delay.

Initialization, probe and scan wait up to 5000 ms by default, polling every
5 ms. power_safe_timeout_ms can override the default. An input read failure
aborts immediately. No module I2C access is made before HIGH is observed.
Errors: -40 startup timeout; -41 missing callback/input read failure;
-42 POWER_SAFE became low. A fault requests enable OFF, marks the module
not ready and emits one structured event for that fault. Hub logs the reason.
Probe/scan cleanup still attempts to restore temporary enable to OFF.

Normal module operations check POWER_SAFE at entry; initialization and
multi-device diagnostics also check between stages/devices. Periodic polling
checks it regardless of IRQ state. Hub raw register commands require both
successful initialization and a live HIGH reading. This is sampled monitoring,
not an edge-latched interrupt: a brief pulse between samples can be missed.
The hardware POWER_SAFE/reset connection remains the immediate reset path.

After correcting the supply condition use `pwm power cycle` to rebuild device
configuration, then `pwm probe`. Probe/scan alone do not restore ready state
after a fault. Intentional power-off does not generate a new POWER_SAFE fault.
The reported HIGH is the level checked during the command, before temporary
power is restored off; it is not a promise that power remains on afterward.

Both project patches are unified diffs generated from captured originals without
Git and support reverse application. Neither patch was applied for validation.
Builds, tests and hardware validation were not run; these remain with the user.

## Polarity and released-output startup (0.12.1)

The SX1509 PWM helper now maps 0..100% directly to 0..255 sink intensity.
Previously it inverted this value, causing logical zero to turn loads fully on.
For the MCP1415 stage, a released/high input produces a low gate output (OFF);
a sinking/low input produces a high gate output (ON). Channel initialization,
stop and normal duty commands therefore share 0% OFF / 100% ON semantics.

PWM setup first makes the pin an input, sets its data latch HIGH, disables its
LED engine, and configures open drain with no internal pulls while still input.
It then loads the timing and intensity registers before enabling the LED engine
and output direction. The final data-latch write starts the requested duty.
At startup that duty is zero, leaving the external pull-up in control.

I2C_ALT_EN_1 remains active high. R1 pulls it low by default; U11 then turns Q13
on to pull LM5168 EN/UVLO low. Hub raises enable deliberately, waits for
POWER_SAFE, and configures the channels OFF. No enable-polarity change is needed.

This correction is in ZapEE only; Hub 0.3.8 uses the updated local source.
The 0.12.1 patch is incremental from 0.12.0. Build and hardware retest not run.
