// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zaptronics/drivers/ad5282.h"
#include "zaptronics/platform/i2c.h"
#include "zaptronics/platform/lock.h"
#include "zaptronics/platform/time.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZTM_PSU_HV_DEFAULT_PGOOD_TIMEOUT_MS    300u

/* Guarded range established by the assembled-board sweep on 2026-07-22. */
#define ZTM_PSU_HV_LOWEST_SAFE_CODE                 55u
#define ZTM_PSU_HV_HIGHEST_SAFE_CODE               255u
#define ZTM_PSU_HV_LOWEST_TESTED_CODE ZTM_PSU_HV_LOWEST_SAFE_CODE
#define ZTM_PSU_HV_CAL_MIN_DRIVE_UV             3922500u
#define ZTM_PSU_HV_CAL_MAX_DRIVE_UV            13695600u
#define ZTM_PSU_HV_CAL_MIN_ESTIMATED_HV_V           1569u
#define ZTM_PSU_HV_CAL_MAX_ESTIMATED_HV_V           5479u
#define ZTM_PSU_HV_SWEEP_STOP_INPUT_UV        14000000u
#define ZTM_PSU_HV_SAFE_RDAC_CODE                   255u

/* XP Power E60 nominal transfer: 0..15 V drive -> 0..6000 V output. */
#define ZTM_PSU_HV_E60_MIN_LINEAR_INPUT_UV       700000u
#define ZTM_PSU_HV_E60_MAX_INPUT_UV            15000000u
#define ZTM_PSU_HV_E60_FULL_SCALE_OUTPUT_MV     6000000u

typedef enum {
    ZTM_PSU_HV_INIT_NOT_STARTED = 0,
    ZTM_PSU_HV_INIT_SAFE_CONTROLS,
    ZTM_PSU_HV_INIT_I2C,
    ZTM_PSU_HV_INIT_AD5282_PROBE,
    ZTM_PSU_HV_INIT_SAFE_RDACS,
    ZTM_PSU_HV_INIT_VERIFY_RDACS,
    ZTM_PSU_HV_INIT_STATUS,
    ZTM_PSU_HV_INIT_READY,
} ztm_psu_hv_init_stage_t;

typedef enum {
    ZTM_PSU_HV_OP_NONE = 0,
    ZTM_PSU_HV_OP_VALIDATE_CODE,
    ZTM_PSU_HV_OP_FORCE_OFF,
    ZTM_PSU_HV_OP_WRITE_RDAC1,
    ZTM_PSU_HV_OP_WRITE_RDAC2,
    ZTM_PSU_HV_OP_VERIFY_RDACS,
    ZTM_PSU_HV_OP_ENABLE,
    ZTM_PSU_HV_OP_WAIT_PGOOD,
    ZTM_PSU_HV_OP_READ_VSENSE,
    ZTM_PSU_HV_OP_VALIDATE_E60_INPUT,
    ZTM_PSU_HV_OP_COMPLETE,
} ztm_psu_hv_operation_stage_t;

typedef enum {
    ZTM_PSU_HV_EVENT_READY = 0,
    ZTM_PSU_HV_EVENT_PGOOD_FAULT,
    ZTM_PSU_HV_EVENT_VSENSE_FAULT,
    ZTM_PSU_HV_EVENT_E60_OVERVOLTAGE,
} ztm_psu_hv_event_type_t;

typedef struct {
    ztm_psu_hv_event_type_t type;
    uint32_t detail;
} ztm_psu_hv_event_t;

typedef struct {
    bool enable_valid;
    bool enabled;
    bool pgood_valid;
    bool pgood;
    bool rdac_readback_valid;
    uint8_t rdac1_code;
    uint8_t rdac2_code;
    bool vsense_valid;
    uint16_t vsense_adc_code;
    uint32_t drive_uV;
    uint32_t estimated_hv_mV;
    bool e60_input_overvoltage;
} ztm_psu_hv_snapshot_t;

typedef struct {
    bool controls_prepared;
    bool forced_off;
    bool ad5282_present;
    uint8_t i2c_address;
    bool rdac_readback_valid;
    uint8_t rdac1_code;
    uint8_t rdac2_code;
    bool enable_valid;
    bool enabled;
    bool pgood_valid;
    bool pgood;
    bool vsense_valid;
    uint16_t vsense_adc_code;
    uint32_t drive_uV;
    uint32_t estimated_hv_mV;
    bool e60_input_overvoltage;
} ztm_psu_hv_probe_result_t;

typedef int (*ztm_psu_hv_prepare_fn)(void *context);
typedef int (*ztm_psu_hv_enable_set_fn)(void *context, bool enabled);
typedef int (*ztm_psu_hv_enable_get_fn)(void *context, bool *enabled);
typedef int (*ztm_psu_hv_pgood_read_fn)(void *context, bool *pgood);
typedef int (*ztm_psu_hv_vsense_read_fn)(void *context,
                                         uint16_t *adc_code,
                                         uint32_t *drive_uV);
typedef void (*ztm_psu_hv_event_fn)(void *context, const ztm_psu_hv_event_t *event);

typedef struct {
    const zap_i2c_bus_t *i2c;
    const zap_time_t *time;
    const zap_lock_t *lock;
    uint8_t address_straps;

    void *control_context;
    ztm_psu_hv_prepare_fn prepare;
    ztm_psu_hv_enable_set_fn enable_set;
    ztm_psu_hv_enable_get_fn enable_get;
    ztm_psu_hv_pgood_read_fn pgood_read;
    ztm_psu_hv_vsense_read_fn vsense_read;

    void *event_context;
    ztm_psu_hv_event_fn event;
} ztm_psu_hv_config_t;

typedef struct {
    ad5282_t potentiometer;
    ztm_psu_hv_config_t config;
    uint8_t i2c_address;

    bool ready;
    bool enabled;
    bool pgood;
    bool pgood_valid;
    uint8_t staged_code;
    uint8_t rdac1_code;
    uint8_t rdac2_code;
    bool rdac_readback_valid;
    uint16_t vsense_adc_code;
    uint32_t drive_uV;
    uint32_t estimated_hv_mV;
    bool vsense_valid;

    ztm_psu_hv_init_stage_t last_init_stage;
    int last_init_rc;
    ztm_psu_hv_operation_stage_t last_operation_stage;
    int last_operation_rc;

    uint32_t poll_count;
    uint32_t fault_events;
    uint32_t pgood_fault_events;
    uint32_t vsense_fault_events;
    uint32_t e60_overvoltage_fault_events;
    bool pgood_fault_active;
    bool vsense_fault_active;
    bool e60_overvoltage_fault_active;

    uint8_t last_start_code;
    uint32_t last_start_wait_ms;
    bool last_start_pgood_read_valid;
    bool last_start_pgood;
    bool last_start_vsense_valid;
    uint16_t last_start_vsense_adc_code;
    uint32_t last_start_drive_uV;
    uint32_t last_start_peak_drive_uV;
} ztm_psu_hv_t;

int ztm_psu_hv_init(ztm_psu_hv_t *device, const ztm_psu_hv_config_t *config);
int ztm_psu_hv_deinit(ztm_psu_hv_t *device);
int ztm_psu_hv_poll(ztm_psu_hv_t *device);
bool ztm_psu_hv_ready(const ztm_psu_hv_t *device);
const char *ztm_psu_hv_init_stage_name(ztm_psu_hv_init_stage_t stage);
const char *ztm_psu_hv_operation_stage_name(ztm_psu_hv_operation_stage_t stage);

bool ztm_psu_hv_stage_code(ztm_psu_hv_t *device, uint8_t voltage_code);
bool ztm_psu_hv_start(ztm_psu_hv_t *device,
                      uint8_t voltage_code,
                      uint32_t pgood_timeout_ms);
bool ztm_psu_hv_stop(ztm_psu_hv_t *device);
bool ztm_psu_hv_read_snapshot(ztm_psu_hv_t *device, ztm_psu_hv_snapshot_t *snapshot);
bool ztm_psu_hv_read_drive_voltage(ztm_psu_hv_t *device,
                                   uint16_t *adc_code,
                                   uint32_t *drive_uV);
uint32_t ztm_psu_hv_estimate_e60_output_mV(uint32_t drive_uV);
bool ztm_psu_hv_e60_input_within_rating(uint32_t drive_uV);

bool ztm_psu_hv_calibrated_drive_for_code(uint8_t code, uint32_t *drive_uV);
bool ztm_psu_hv_select_code_for_estimated_hv(uint32_t target_hv_V,
                                              uint8_t *code,
                                              uint32_t *calibrated_drive_uV,
                                              uint32_t *calibrated_hv_mV);

bool ztm_psu_hv_read_pgood(ztm_psu_hv_t *device, bool *pgood);
bool ztm_psu_hv_read_enable(ztm_psu_hv_t *device, bool *enabled);
int ztm_psu_hv_probe(ztm_psu_hv_t *device, ztm_psu_hv_probe_result_t *result);
bool ztm_psu_hv_read_rdac(ztm_psu_hv_t *device,
                          ad5282_channel_t channel,
                          uint8_t *code);
bool ztm_psu_hv_write_rdac_off(ztm_psu_hv_t *device,
                               ad5282_channel_t channel,
                               uint8_t code);

#ifdef __cplusplus
}
#endif
