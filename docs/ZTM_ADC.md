# ZTM_ADC

`zap::ztm_adc` is the portable board library for the Zaptronics temperature ADC
module built around the AD7124-8.

## Measurement topology

The current board definition exposes:

- 3 floating thermocouple inputs;
- 1 Pt1000 cold-junction sensor;
- 8 external Pt1000 RTD inputs;
- AD7124 internal 2.5 V reference for thermocouples;
- ratiometric RTD measurement against the fitted 2 kohm reference resistor;
- 500 uA one-shot excitation for RTD measurements.

The board library owns this topology. The `zap::ad7124` driver remains generic.

## Dependencies

```text
zap::ztm_adc
  +-- zap::ad7124
  |     `-- zap::spi
  `-- zap::thermocouple
```

The thermocouple implementation contains the NIST reference-function
coefficients for B, E, J, K, N, R, S and T types. ZTM_ADC defaults all three
inputs to Type-K but lets the application select a type per channel.

## Portable configuration

```c
ztm_adc_t adc = {0};
ztm_adc_config_t config = {
    .spi = &my_spi_device,
    .time = &my_time,
    .lock = &my_optional_operation_lock,
    .event = my_event_handler,
    .event_context = my_context,
};

int rc = ztm_adc_init(&adc, &config);
```

`spi` and a high-resolution `time` provider are required. `lock` and `event` are
optional. Omitting `lock` means the application guarantees that high-level
ZTM_ADC operations are not entered concurrently.

## SPI transaction contract

AD7124 register reads require chip-select to stay active while the communication
register byte is written and the response bytes are clocked back. ZapEE models
this explicitly with:

```text
zap_spi_begin()
  zap_spi_transfer(command)
  zap_spi_transfer(response)
zap_spi_end()
```

The platform adapter owns controller configuration, locking and chip-select
implementation. This allows direct GPIO CS, a motherboard CS decoder, or a
Zephyr `spi_dt_spec` without changing the AD7124 driver.

## Reading sensors

```c
ztm_adc_rtd_sample_t rtd;
if (ztm_adc_read_rtd(&adc, 1, &rtd) && rtd.status == ZTM_ADC_SENSOR_OK) {
    use_temperature(rtd.temperature_c);
}

ztm_adc_tc_sample_t tc;
ztm_adc_set_thermocouple_type(&adc, 1, THERMOCOUPLE_TYPE_K);
if (ztm_adc_read_thermocouple(&adc, 1, &tc) && tc.status == ZTM_ADC_SENSOR_OK) {
    use_temperature(tc.temperature_c);
}
```

Raw ADC codes of `0x000000` and `0xFFFFFF` are reported as disconnected rather
than converted into temperatures.

## Cold-junction compensation

Each thermocouple read first measures the on-board Pt1000 cold-junction sensor.
The measured thermocouple differential voltage is then compensated using the
selected NIST thermocouple type. If the cold-junction measurement is unavailable,
the SPI transaction can still succeed and the sample reports
`ZTM_ADC_SENSOR_CJC_UNAVAILABLE`.

## Status and diagnostics

Applications should use `ztm_adc_get_status()` rather than reading `ztm_adc_t`
fields directly. The instance struct remains visible so embedded applications can
allocate it statically without a heap, but its internal layout is not the
diagnostic API contract.

```c
ztm_adc_status_t status;
if (ztm_adc_get_status(&adc, &status)) {
    if (!status.configuration_verified) {
        service_configuration_fault();
    }
    if (status.last_event_valid) {
        record_event(status.last_event.type);
    }
}
```

The snapshot is copied while holding the optional module operation lock and
includes readiness, AD7124 device ID, configuration/POR state, reference and
excitation settings, all three thermocouple types, error counters/mask, the last
structured event, and the last measurement diagnostic record. This makes it
suitable for CLIs, telemetry and service diagnostics without coupling them to the
mutable module instance.

`ztm_adc_get_last_measurement()` remains available as a smaller convenience
accessor when only the most recent measurement trace is required.

## Locking and thread safety

The module performs multi-register sequences during measurements, calibration,
configuration verification, shutdown and diagnostics. The optional `zap_lock_t`
protects those complete operations, not merely individual SPI transactions.
`poll()` uses a non-blocking lock attempt so background diagnostics skip a cycle
rather than stalling an active measurement. State/diagnostic accessors also use
the operation lock when one is supplied.

A NULL lock is valid and means the application guarantees that the instance is
not entered concurrently. Lock callbacks execute only in normal caller/thread
context; ZapEE does not require or assume ISR-safe mutex primitives.

## Platform timeout semantics

`ZTM_ADC_CONVERSION_TIMEOUT_US` is enforced portably using the monotonic
`zap_time_t` clock. `zap_lock_t.timeout_us` limits how long an operation waits to
acquire the optional operation lock. The `zap_spi_device_t.timeout_us` value is
forwarded to the platform SPI adapter, but vendor SDKs with only blocking SPI
APIs may own the actual transfer timeout policy. Platform adapter documentation
identifies that limitation rather than claiming a timeout guarantee the SDK
cannot provide.

## Structured events

The portable library never prints. Optional events report:

- ready;
- unexpected AD7124 reset/POR;
- newly asserted AD7124 error bits;
- communication failures; and
- configuration-register mismatches.

The consuming application decides whether those become console logs, alarms,
telemetry, RTOS messages, or are ignored. Event callbacks are synchronous and
may be invoked while the module's operation lock is held; a callback must not
re-enter the same `ztm_adc_t` instance. Queue/defer work if re-entry is required.
