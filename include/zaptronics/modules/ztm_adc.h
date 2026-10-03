// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zaptronics/drivers/ad7124.h"
#include "zaptronics/drivers/thermocouple.h"
#include "zaptronics/platform/lock.h"
#include "zaptronics/platform/spi.h"
#include "zaptronics/platform/time.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZTM_ADC_TC_COUNT                 3u
#define ZTM_ADC_RTD_COUNT                8u
#define ZTM_ADC_SENSOR_CHANNEL_COUNT    12u

#define ZTM_ADC_SETUP_TC                 0u
#define ZTM_ADC_SETUP_RTD                1u
#define ZTM_ADC_CHANNEL_TC_FIRST         0u
#define ZTM_ADC_CHANNEL_CJC              3u
#define ZTM_ADC_CHANNEL_RTD_FIRST        4u

#define ZTM_ADC_AIN_CJC                  6u
#define ZTM_ADC_AIN_RTD_FIRST            7u
#define ZTM_ADC_AIN_RTD_RETURN          15u

#define ZTM_ADC_RTD_R0_OHMS           1000.0
#define ZTM_ADC_EXTERNAL_RTD_SERIES_OHMS 100.0
#define ZTM_ADC_RREF_OHMS             2000.0
#define ZTM_ADC_INTERNAL_REF_VOLTS       2.5
#define ZTM_ADC_RTD_EXCITATION        AD7124_IOUT_500UA
#define ZTM_ADC_FILTER_FS              200u
#define ZTM_ADC_CONVERSION_TIMEOUT_US 2000000u
#define ZTM_ADC_IOUT_SETTLE_US          2000u
#define ZTM_ADC_OPERATION_TIMEOUT_US  2100000u

#define ZTM_ADC_TC_VBIAS_MASK ((uint16_t)((1u << 1) | (1u << 3) | (1u << 5)))

typedef enum {
    ZTM_ADC_SENSOR_OK = 0,
    ZTM_ADC_SENSOR_DISCONNECTED,
    ZTM_ADC_SENSOR_CJC_UNAVAILABLE,
    ZTM_ADC_SENSOR_OUT_OF_RANGE,
} ztm_adc_sensor_status_t;

typedef struct {
    bool valid;
    ztm_adc_sensor_status_t status;
    uint8_t logical_index;
    uint8_t ad7124_channel;
    uint8_t ain_positive;
    uint32_t raw_code;
    double resistance_ohms;
    double temperature_c;
} ztm_adc_rtd_sample_t;

typedef struct {
    bool valid;
    ztm_adc_sensor_status_t status;
    uint8_t logical_index;
    uint8_t ad7124_channel;
    uint8_t ain_positive;
    uint8_t ain_negative;
    uint32_t raw_code;
    thermocouple_type_t type;
    double differential_mv;
    bool cold_junction_valid;
    double cold_junction_c;
    double compensated_mv;
    double temperature_c;
} ztm_adc_tc_sample_t;

typedef enum {
    ZTM_ADC_MEASURE_NONE = 0,
    ZTM_ADC_MEASURE_IOUT_ENABLE,
    ZTM_ADC_MEASURE_CHANNEL_ENABLE,
    ZTM_ADC_MEASURE_MODE_START,
    ZTM_ADC_MEASURE_MODE_VERIFY,
    ZTM_ADC_MEASURE_READY_TIMEOUT,
    ZTM_ADC_MEASURE_DATA_READ,
    ZTM_ADC_MEASURE_CHANNEL_MISMATCH,
    ZTM_ADC_MEASURE_STANDBY,
    ZTM_ADC_MEASURE_CHANNEL_DISABLE,
    ZTM_ADC_MEASURE_IOUT_DISABLE,
    ZTM_ADC_MEASURE_SENSOR_RANGE,
    ZTM_ADC_MEASURE_OK,
} ztm_adc_measure_stage_t;

typedef struct {
    ztm_adc_measure_stage_t stage;
    uint8_t expected_channel;
    uint8_t active_channel;
    uint8_t status_before;
    uint8_t status_ready;
    uint8_t status_after_cleanup;
    uint16_t channel_register;
    uint16_t adc_control_after_start;
    uint16_t adc_control_at_result;
    uint16_t adc_control_after_cleanup;
    uint32_t io_control_1;
    uint32_t filter_register;
    uint32_t elapsed_us;
    uint32_t ready_polls;
    uint32_t raw_code;
    uint32_t error_mask;
    double resistance_ohms;
} ztm_adc_measure_diag_t;

typedef enum {
    ZTM_ADC_EVENT_READY = 0,
    ZTM_ADC_EVENT_UNEXPECTED_RESET,
    ZTM_ADC_EVENT_ADC_ERROR,
    ZTM_ADC_EVENT_COMMUNICATION_ERROR,
    ZTM_ADC_EVENT_CONFIGURATION_MISMATCH,
} ztm_adc_event_type_t;

typedef struct {
    ztm_adc_event_type_t type;
    uint8_t status;
    uint8_t register_address;
    uint8_t register_width;
    uint32_t error_mask;
    uint32_t expected;
    uint32_t actual;
} ztm_adc_event_t;


typedef struct {
    bool ready;
    uint8_t device_id;
    bool configuration_verified;
    bool por_fault_active;

    double reference_resistor_ohms;
    ad7124_iout_val_t rtd_excitation;
    thermocouple_type_t thermocouple_types[ZTM_ADC_TC_COUNT];

    uint32_t last_error_mask;
    uint32_t error_event_count;
    uint32_t communication_error_count;
    uint32_t configuration_error_count;

    bool last_event_valid;
    ztm_adc_event_t last_event;
    ztm_adc_measure_diag_t last_measurement;
} ztm_adc_status_t;

typedef void (*ztm_adc_event_fn)(void *context, const ztm_adc_event_t *event);

typedef struct {
    const zap_spi_device_t *spi;
    const zap_time_t *time;
    const zap_lock_t *lock;
    void *event_context;
    ztm_adc_event_fn event;
} ztm_adc_config_t;

/*
 * Instance storage is public so applications can allocate it statically. Treat
 * the fields as implementation detail; use the public accessors, especially
 * ztm_adc_get_status(), rather than coupling application code to this layout.
 */
typedef struct {
    ad7124_dev_t adc;
    ztm_adc_config_t config;
    bool ready;
    uint8_t device_id;

    double reference_resistor_ohms;
    ad7124_iout_val_t rtd_excitation;
    thermocouple_type_t thermocouple_types[ZTM_ADC_TC_COUNT];

    uint32_t last_error_mask;
    uint32_t error_event_count;
    uint32_t communication_error_count;
    uint32_t configuration_error_count;
    bool configuration_verified;
    bool por_fault_active;
    bool last_event_valid;
    ztm_adc_event_t last_event;
    ztm_adc_measure_diag_t last_measurement;
} ztm_adc_t;

int ztm_adc_init(ztm_adc_t *device, const ztm_adc_config_t *config);
int ztm_adc_poll(ztm_adc_t *device);
int ztm_adc_deinit(ztm_adc_t *device);
bool ztm_adc_ready(ztm_adc_t *device);

bool ztm_adc_read_cold_junction(ztm_adc_t *device, ztm_adc_rtd_sample_t *sample);
bool ztm_adc_read_rtd(ztm_adc_t *device, uint8_t rtd_1based, ztm_adc_rtd_sample_t *sample);
bool ztm_adc_read_all_rtds(ztm_adc_t *device,
                            ztm_adc_rtd_sample_t *cold_junction,
                            ztm_adc_rtd_sample_t samples[ZTM_ADC_RTD_COUNT]);

bool ztm_adc_set_thermocouple_type(ztm_adc_t *device, uint8_t tc_1based, thermocouple_type_t type);
bool ztm_adc_get_thermocouple_type(ztm_adc_t *device, uint8_t tc_1based, thermocouple_type_t *type);
bool ztm_adc_read_thermocouple(ztm_adc_t *device, uint8_t tc_1based, ztm_adc_tc_sample_t *sample);
bool ztm_adc_read_all_thermocouples(ztm_adc_t *device, ztm_adc_tc_sample_t samples[ZTM_ADC_TC_COUNT]);

bool ztm_adc_read_all(ztm_adc_t *device,
                      ztm_adc_rtd_sample_t *cold_junction,
                      ztm_adc_rtd_sample_t rtds[ZTM_ADC_RTD_COUNT],
                      ztm_adc_tc_sample_t thermocouples[ZTM_ADC_TC_COUNT]);

bool ztm_adc_read_error_status(ztm_adc_t *device, uint8_t *status, uint32_t *error_mask);
bool ztm_adc_verify_configuration(ztm_adc_t *device);
bool ztm_adc_read_register(ztm_adc_t *device, uint8_t address, uint8_t width_bytes, uint32_t *value);
bool ztm_adc_calibrate_tc_setup(ztm_adc_t *device);
bool ztm_adc_calibrate_rtd_setup(ztm_adc_t *device);
bool ztm_adc_calibrate_all(ztm_adc_t *device);
bool ztm_adc_iout_off(ztm_adc_t *device);

/*
 * Copy a coherent, operation-locked snapshot of the module's public diagnostic
 * state. Applications should prefer this accessor over reading ztm_adc_t
 * fields directly; the struct layout is public only so callers can allocate
 * instances without dynamic memory.
 */
bool ztm_adc_get_status(ztm_adc_t *device, ztm_adc_status_t *status);

/* Convenience accessor retained for focused measurement diagnostics. */
bool ztm_adc_get_last_measurement(ztm_adc_t *device, ztm_adc_measure_diag_t *diag);

bool ztm_adc_pt1000_temperature(double resistance_ohms, double *temperature_c);
double ztm_adc_rtd_resistance_from_code(uint32_t code, double reference_resistor_ohms);

#ifdef __cplusplus
}
#endif
