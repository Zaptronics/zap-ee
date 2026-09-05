# ZTM_STEPPER portability plan

This is the pre-extraction design for the Hub's current ZTM_STEPPER/L6470 code.
No production ZTM_STEPPER source is moved into ZapEE in 0.4.1; the purpose of
this document is to lock down the transport boundary before changing working
motor-control firmware.

## Current Hub topology

The present board contains four L6470 devices in one SPI daisy chain. Drivers
1..3 are active motor channels; driver 4 is intentionally unused and must remain
in hard HiZ.

The existing Hub implementation currently owns all of these concerns together:

- RP2350/Pico SPI types and L6470 SPI mode/baud configuration;
- motherboard module-enable control;
- the Hub `bus_spi_chain_t`/slot/target implementation;
- L6470 register/command encoding;
- ZTM_STEPPER defaults and runtime KVAL/microstep state;
- one-millisecond reset settling delays;
- fault polling and console logging;
- the global `ztm_stepper_1` instance.

Those responsibilities should be separated before ZTM_STEPPER becomes public
ZapEE API.

## Important transport difference from ZTM_ADC

ZTM_ADC's AD7124 requires chip-select to remain asserted across command and data
phases, which is exactly what `zap_spi_device_t begin/transfer/end` models.

The L6470 chain is different. A command consists of a series of one-byte chain
**frames**, and chip-select must pulse between those bytes so every L6470 shifts
and latches one command byte per frame. The Hub therefore holds ownership of the
shared SPI fabric across the sequence but its `bus_spi_chain_frame()` asserts and
deselects CS for each frame.

Consequently ZTM_STEPPER must **not** be forced through the existing
`zap_spi_device_t` abstraction. Doing so would hide a real electrical/protocol
difference and could generate incorrect wire traffic.

## Proposed portable framed-SPI contract

Before extracting L6470, add a small portable framed transaction contract with
semantics equivalent to:

```text
sequence_begin()       reserve/configure the shared SPI fabric
  frame(...)           assert CS, transfer one complete frame, deassert CS
  frame(...)
  frame(...)
sequence_end()         release the shared SPI fabric
```

A likely public shape is a `zap_spi_framed_device_t` or similarly named target
with callbacks for `sequence_begin`, `frame`, and `sequence_end`. The exact name
should be chosen when implementation begins; the semantic distinction from
`zap_spi_device_t` is the important part.

The generic Pico adapter can implement this with one mutex/controller plus a
normal GPIO CS. The Hub adapter can map it directly to its existing
`bus_spi_claim()` / `bus_spi_frame_locked()` / `bus_spi_release()` primitives.
A Zephyr adapter should only be offered if the selected SPI/controller API can
faithfully provide the required CS pulse-per-frame behavior while retaining
sequence-level serialization.

## Chain ownership belongs in ZTM_STEPPER

The public ZTM_STEPPER board library should know that this board has four
one-byte L6470 slots. It can compose each physical chain frame itself:

```text
wire order: farthest driver ... nearest driver
untargeted slot: L6470 NOP
selected slot: command/data byte
```

This is preferable to exporting the Hub's general-purpose heterogeneous
`bus_spi_chain_t` machinery into ZapEE. The module topology is board knowledge;
the platform adapter only needs to provide a correctly framed physical SPI
endpoint.

Broadcast and active-driver operations then become ordinary portable frame
composition. The existing optimized `run_all` behavior can be preserved by
sending each of the four RUN bytes as a separate four-slot physical frame.

## L6470 driver boundary

The L6470 silicon layer should lose all `ztmb/bus.h`, Pico SPI enum, and chain
slot types. It should retain:

- command/register constants;
- status/fault decoding;
- speed/acceleration conversion helpers;
- RUN/MOVE and parameter encoding/decoding;
- reusable single-device command helpers built on the new framed-SPI endpoint
  if that remains useful outside ZTM_STEPPER.

Codec/unit-conversion functions are straightforward host-test candidates.

## Proposed ZTM_STEPPER public shape

Like the other ZapEE modules, ZTM_STEPPER should use explicit instances:

```c
ztm_stepper_t stepper = {0};
ztm_stepper_config_t config = {
    .spi = &my_framed_spi,
    .time = &my_time,
    .lock = &my_optional_operation_lock,
    .power_set = my_module_power,
    .event = my_stepper_event,
};

ztm_stepper_init(&stepper, &config);
ztm_stepper_run(&stepper, 1, true, 200.0f);
```

The configuration should contain no Pico/Zephyr/Hub-native types.

## Concurrency

The existing Hub bus lock serializes physical SPI traffic but does not protect a
complete high-level operation such as hard-HiZ -> read STEP_MODE -> write
STEP_MODE -> reset position. ZapEE should add the same optional operation-level
`zap_lock_t` pattern used by ZTM_ADC so those sequences cannot interleave in a
threaded application.

`poll()` should use a non-blocking lock attempt and skip a fault-poll cycle when
a motion/configuration operation owns the module.

## Timing and board control

Replace direct `busy_wait_ms(1)` with `zap_time_t`. The portable module should
request the one-millisecond L6470 reset settle delay; the platform decides how
that delay is implemented.

Module enable should be an optional board-control callback rather than a Hub
I/O-expander or Pico GPIO assumption.

## Fault reporting

Replace direct `log_printf()` calls with structured events. At minimum expose:

- stall assertion/clear per driver;
- thermal warning/shutdown assertion/clear per driver;
- communication/transport failure;
- invalid configuration or reinitialization failure.

A service/status snapshot should expose the stored KVAL and microstep settings,
last raw L6470 statuses and fault-latched state without requiring callers to
inspect `ztm_stepper_t` internals.

## Behavior to preserve

The migration must preserve the currently tested defaults and safety behavior:

- four L6470 devices in the physical chain;
- only drivers 1..3 may run/move;
- driver 4 remains hard HiZ;
- default KVAL HOLD=0, RUN=12, ACC=12, DEC=12;
- default microstep mode = 1/4;
- gentle acceleration/deceleration and 500 step/s configured max speed;
- reset followed by configuration restore;
- existing status/stall/thermal detection semantics.

## Extraction order

1. Add and host-test the framed-SPI portable contract.
2. Extract L6470 constants/codecs and test command/status encoding.
3. Implement direct Pico and Hub framed-SPI adapters.
4. Extract explicit-instance ZTM_STEPPER with operation locking/events/time.
5. Add chain-frame composition tests, including slot ordering and NOP fill.
6. Make the Hub consume a local `zap::ztm_stepper` while retaining compatibility
   wrappers for its existing CLI/application API.
7. Hardware-regression test all four physical slots, run/move/stop, KVAL,
   microstep changes, reset/config restore, and fault polling.
8. Remove the old Hub production implementation and tag the ZapEE release.
