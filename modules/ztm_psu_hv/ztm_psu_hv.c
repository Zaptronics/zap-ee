// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/modules/ztm_psu_hv.h"

#include <stddef.h>
#include <string.h>

typedef struct {
    uint8_t code;
    uint32_t drive_uV;
} ztm_psu_hv_cal_point_t;

/* Measured on the assembled ZTM_PSU_HV board by the guarded 0.2.10 sweep. */
static const ztm_psu_hv_cal_point_t g_hv_calibration[] = {
    {255u,  3922500u}, {245u,  4043400u}, {235u,  4170300u},
    {225u,  4321400u}, {215u,  4430200u}, {205u,  4611500u},
    {195u,  4804900u}, {185u,  4950000u}, {175u,  5167600u},
    {165u,  5457700u}, {155u,  5705500u}, {145u,  6001600u},
    {135u,  6370300u}, {125u,  6805500u}, {115u,  7264800u},
    {105u,  7857100u}, { 95u,  8522000u}, { 85u,  9368100u},
    { 75u, 10431900u}, { 65u, 11822000u}, { 55u, 13695600u},
};

#define HV_CAL_POINT_COUNT (sizeof(g_hv_calibration) / sizeof(g_hv_calibration[0]))

static bool config_valid(const ztm_psu_hv_config_t *config)
{
    return config != NULL &&
           zap_i2c_bus_valid(config->i2c) &&
           zap_time_valid(config->time) &&
           zap_lock_valid(config->lock) &&
           config->prepare != NULL &&
           config->enable_set != NULL &&
           config->enable_get != NULL &&
           config->pgood_read != NULL &&
           config->vsense_read != NULL;
}

static bool device_bound(const ztm_psu_hv_t *device)
{
    return device != NULL && config_valid(&device->config);
}

static bool lock_op(ztm_psu_hv_t *device)
{
    return device != NULL && zap_lock_acquire(device->config.lock) == 0;
}

static bool try_lock_op(ztm_psu_hv_t *device)
{
    return device != NULL && zap_lock_try_acquire(device->config.lock) == 0;
}

static void unlock_op(ztm_psu_hv_t *device)
{
    if (device != NULL) {
        zap_lock_release(device->config.lock);
    }
}

static void emit(ztm_psu_hv_t *device, ztm_psu_hv_event_type_t type, uint32_t detail)
{
    if (device != NULL && device->config.event != NULL) {
        const ztm_psu_hv_event_t event = { .type = type, .detail = detail };
        device->config.event(device->config.event_context, &event);
    }
}

const char *ztm_psu_hv_init_stage_name(ztm_psu_hv_init_stage_t stage)
{
    switch (stage) {
        case ZTM_PSU_HV_INIT_SAFE_CONTROLS: return "safe controls";
        case ZTM_PSU_HV_INIT_I2C: return "AD5282 I2C setup";
        case ZTM_PSU_HV_INIT_AD5282_PROBE: return "AD5282 probe";
        case ZTM_PSU_HV_INIT_SAFE_RDACS: return "set safe RDACs";
        case ZTM_PSU_HV_INIT_VERIFY_RDACS: return "verify RDACs";
        case ZTM_PSU_HV_INIT_STATUS: return "read safe-state status";
        case ZTM_PSU_HV_INIT_READY: return "ready";
        default: return "not started";
    }
}

const char *ztm_psu_hv_operation_stage_name(ztm_psu_hv_operation_stage_t stage)
{
    switch (stage) {
        case ZTM_PSU_HV_OP_VALIDATE_CODE: return "validate operating code";
        case ZTM_PSU_HV_OP_FORCE_OFF: return "force enable low";
        case ZTM_PSU_HV_OP_WRITE_RDAC1: return "write RDAC1";
        case ZTM_PSU_HV_OP_WRITE_RDAC2: return "write RDAC2";
        case ZTM_PSU_HV_OP_VERIFY_RDACS: return "verify RDAC readback";
        case ZTM_PSU_HV_OP_ENABLE: return "assert enable";
        case ZTM_PSU_HV_OP_WAIT_PGOOD: return "wait for power-good";
        case ZTM_PSU_HV_OP_READ_VSENSE: return "read E60 drive VSENSE";
        case ZTM_PSU_HV_OP_VALIDATE_E60_INPUT: return "validate E60 input";
        case ZTM_PSU_HV_OP_COMPLETE: return "complete";
        default: return "none";
    }
}

static int init_failed(ztm_psu_hv_t *device, ztm_psu_hv_init_stage_t stage, int rc)
{
    if (device != NULL) {
        device->ready = false;
        device->enabled = false;
        device->last_init_stage = stage;
        device->last_init_rc = rc;
        if (device->config.enable_set != NULL) {
            (void)device->config.enable_set(device->config.control_context, false);
        }
    }
    return rc;
}

static bool set_operation_result(ztm_psu_hv_t *device,
                                 ztm_psu_hv_operation_stage_t stage,
                                 int rc)
{
    if (device != NULL) {
        device->last_operation_stage = stage;
        device->last_operation_rc = rc;
    }
    return rc == 0;
}

static bool read_enable_locked(ztm_psu_hv_t *device, bool *enabled)
{
    return device != NULL && enabled != NULL &&
           device->config.enable_get(device->config.control_context, enabled) == 0;
}

static bool read_pgood_locked(ztm_psu_hv_t *device, bool *pgood)
{
    return device != NULL && pgood != NULL &&
           device->config.pgood_read(device->config.control_context, pgood) == 0;
}

static bool enable_locked(ztm_psu_hv_t *device, bool state)
{
    if (device == NULL ||
        device->config.enable_set(device->config.control_context, state) != 0) {
        return false;
    }
    device->enabled = state;
    return true;
}

uint32_t ztm_psu_hv_estimate_e60_output_mV(uint32_t drive_uV)
{
    uint64_t numerator = (uint64_t)drive_uV *
                         (uint64_t)ZTM_PSU_HV_E60_FULL_SCALE_OUTPUT_MV;
    numerator += ZTM_PSU_HV_E60_MAX_INPUT_UV / 2u;
    return (uint32_t)(numerator / ZTM_PSU_HV_E60_MAX_INPUT_UV);
}

bool ztm_psu_hv_e60_input_within_rating(uint32_t drive_uV)
{
    return drive_uV <= ZTM_PSU_HV_E60_MAX_INPUT_UV;
}

bool ztm_psu_hv_calibrated_drive_for_code(uint8_t code, uint32_t *drive_uV)
{
    if (drive_uV == NULL || code < ZTM_PSU_HV_LOWEST_SAFE_CODE) {
        return false;
    }
    if (code >= g_hv_calibration[0].code) {
        *drive_uV = g_hv_calibration[0].drive_uV;
        return true;
    }

    for (size_t i = 0u; i + 1u < HV_CAL_POINT_COUNT; ++i) {
        const ztm_psu_hv_cal_point_t *high_code = &g_hv_calibration[i];
        const ztm_psu_hv_cal_point_t *low_code = &g_hv_calibration[i + 1u];
        if (code <= high_code->code && code >= low_code->code) {
            const uint32_t code_span = (uint32_t)high_code->code - (uint32_t)low_code->code;
            const uint32_t code_offset = (uint32_t)high_code->code - (uint32_t)code;
            const uint32_t voltage_span = low_code->drive_uV - high_code->drive_uV;
            const uint64_t scaled = (uint64_t)voltage_span * code_offset;
            *drive_uV = high_code->drive_uV +
                        (uint32_t)((scaled + code_span / 2u) / code_span);
            return true;
        }
    }
    return false;
}

bool ztm_psu_hv_select_code_for_estimated_hv(uint32_t target_hv_V,
                                              uint8_t *code,
                                              uint32_t *calibrated_drive_uV,
                                              uint32_t *calibrated_hv_mV)
{
    if (code == NULL || calibrated_drive_uV == NULL || calibrated_hv_mV == NULL ||
        target_hv_V < ZTM_PSU_HV_CAL_MIN_ESTIMATED_HV_V ||
        target_hv_V > ZTM_PSU_HV_CAL_MAX_ESTIMATED_HV_V) {
        return false;
    }

    const uint32_t target_drive_uV = target_hv_V * 2500u;
    uint8_t selected = ZTM_PSU_HV_HIGHEST_SAFE_CODE;
    uint32_t selected_drive = ZTM_PSU_HV_CAL_MIN_DRIVE_UV;
    for (int candidate = (int)ZTM_PSU_HV_HIGHEST_SAFE_CODE;
         candidate >= (int)ZTM_PSU_HV_LOWEST_SAFE_CODE;
         --candidate) {
        uint32_t candidate_drive = 0u;
        if (!ztm_psu_hv_calibrated_drive_for_code((uint8_t)candidate, &candidate_drive)) {
            continue;
        }
        if (candidate_drive > target_drive_uV) {
            break;
        }
        selected = (uint8_t)candidate;
        selected_drive = candidate_drive;
    }

    *code = selected;
    *calibrated_drive_uV = selected_drive;
    *calibrated_hv_mV = ztm_psu_hv_estimate_e60_output_mV(selected_drive);
    return true;
}

static bool read_drive_voltage_locked(ztm_psu_hv_t *device,
                                      uint16_t *adc_code,
                                      uint32_t *drive_uV)
{
    if (device == NULL || drive_uV == NULL) {
        return false;
    }

    uint16_t code = 0u;
    uint32_t measured_uV = 0u;
    if (device->config.vsense_read(device->config.control_context,
                                   &code,
                                   &measured_uV) != 0) {
        device->vsense_valid = false;
        return false;
    }

    device->vsense_valid = true;
    device->vsense_adc_code = code;
    device->drive_uV = measured_uV;
    device->estimated_hv_mV = ztm_psu_hv_estimate_e60_output_mV(measured_uV);
    if (adc_code != NULL) {
        *adc_code = code;
    }
    *drive_uV = measured_uV;
    return true;
}

static bool read_rdac_locked(ztm_psu_hv_t *device,
                             ad5282_channel_t channel,
                             uint8_t *code)
{
    return device != NULL &&
           ad5282_read_rdac(&device->potentiometer, channel, code) == AD5282_OK;
}

static bool verify_rdacs_locked(ztm_psu_hv_t *device,
                                uint8_t expected_rdac1,
                                uint8_t expected_rdac2)
{
    uint8_t rdac1 = 0u;
    uint8_t rdac2 = 0u;
    if (!read_rdac_locked(device, AD5282_RDAC1, &rdac1) ||
        !read_rdac_locked(device, AD5282_RDAC2, &rdac2)) {
        device->rdac_readback_valid = false;
        return false;
    }

    device->rdac1_code = rdac1;
    device->rdac2_code = rdac2;
    device->rdac_readback_valid = true;
    return rdac1 == expected_rdac1 && rdac2 == expected_rdac2;
}

static bool stop_locked(ztm_psu_hv_t *device)
{
    if (device == NULL) {
        return false;
    }

    device->last_operation_stage = ZTM_PSU_HV_OP_FORCE_OFF;
    if (!enable_locked(device, false)) {
        return set_operation_result(device, ZTM_PSU_HV_OP_FORCE_OFF, -1);
    }

    device->enabled = false;
    device->pgood = false;
    device->pgood_valid = false;
    device->vsense_valid = false;
    device->vsense_adc_code = 0u;
    device->drive_uV = 0u;
    device->estimated_hv_mV = 0u;

    device->last_operation_stage = ZTM_PSU_HV_OP_WRITE_RDAC1;
    const bool rdac1_ok = ad5282_write_rdac(&device->potentiometer,
                                             AD5282_RDAC1,
                                             ZTM_PSU_HV_SAFE_RDAC_CODE,
                                             false,
                                             false) == AD5282_OK;
    device->last_operation_stage = ZTM_PSU_HV_OP_WRITE_RDAC2;
    const bool rdac2_ok = ad5282_write_rdac(&device->potentiometer,
                                             AD5282_RDAC2,
                                             ZTM_PSU_HV_SAFE_RDAC_CODE,
                                             false,
                                             false) == AD5282_OK;
    device->staged_code = ZTM_PSU_HV_SAFE_RDAC_CODE;

    if (rdac1_ok && rdac2_ok) {
        device->last_operation_stage = ZTM_PSU_HV_OP_VERIFY_RDACS;
        if (!verify_rdacs_locked(device,
                                 ZTM_PSU_HV_SAFE_RDAC_CODE,
                                 ZTM_PSU_HV_SAFE_RDAC_CODE)) {
            return set_operation_result(device, ZTM_PSU_HV_OP_VERIFY_RDACS, -4);
        }
        return set_operation_result(device, ZTM_PSU_HV_OP_COMPLETE, 0);
    }

    device->rdac_readback_valid = false;
    return set_operation_result(device,
                                rdac1_ok ? ZTM_PSU_HV_OP_WRITE_RDAC2
                                         : ZTM_PSU_HV_OP_WRITE_RDAC1,
                                -3);
}

static bool stage_code_locked(ztm_psu_hv_t *device, uint8_t voltage_code)
{
    if (device == NULL || !device->ready) {
        return false;
    }

    device->last_operation_stage = ZTM_PSU_HV_OP_FORCE_OFF;
    if (!enable_locked(device, false)) {
        return set_operation_result(device, ZTM_PSU_HV_OP_FORCE_OFF, -1);
    }

    device->last_operation_stage = ZTM_PSU_HV_OP_WRITE_RDAC1;
    if (ad5282_write_rdac(&device->potentiometer,
                          AD5282_RDAC1,
                          0u,
                          false,
                          false) != AD5282_OK) {
        return set_operation_result(device, ZTM_PSU_HV_OP_WRITE_RDAC1, -2);
    }

    device->last_operation_stage = ZTM_PSU_HV_OP_WRITE_RDAC2;
    if (ad5282_write_rdac(&device->potentiometer,
                          AD5282_RDAC2,
                          voltage_code,
                          false,
                          false) != AD5282_OK) {
        (void)stop_locked(device);
        return set_operation_result(device, ZTM_PSU_HV_OP_WRITE_RDAC2, -3);
    }

    device->last_operation_stage = ZTM_PSU_HV_OP_VERIFY_RDACS;
    if (!verify_rdacs_locked(device, 0u, voltage_code)) {
        (void)stop_locked(device);
        return set_operation_result(device, ZTM_PSU_HV_OP_VERIFY_RDACS, -4);
    }

    device->staged_code = voltage_code;
    return set_operation_result(device, ZTM_PSU_HV_OP_COMPLETE, 0);
}

static bool start_locked(ztm_psu_hv_t *device,
                         uint8_t voltage_code,
                         uint32_t pgood_timeout_ms)
{
    if (device == NULL || !device->ready) {
        return false;
    }

    device->last_start_code = voltage_code;
    device->last_start_wait_ms = 0u;
    device->last_start_pgood_read_valid = false;
    device->last_start_pgood = false;
    device->last_start_vsense_valid = false;
    device->last_start_vsense_adc_code = 0u;
    device->last_start_drive_uV = 0u;
    device->last_start_peak_drive_uV = 0u;

    device->last_operation_stage = ZTM_PSU_HV_OP_VALIDATE_CODE;
    if (voltage_code < ZTM_PSU_HV_LOWEST_TESTED_CODE) {
        (void)stop_locked(device);
        return set_operation_result(device, ZTM_PSU_HV_OP_VALIDATE_CODE, -9);
    }

    if (!stage_code_locked(device, voltage_code)) {
        return false;
    }

    device->last_operation_stage = ZTM_PSU_HV_OP_ENABLE;
    if (!enable_locked(device, true)) {
        (void)stop_locked(device);
        return set_operation_result(device, ZTM_PSU_HV_OP_ENABLE, -5);
    }

    device->last_operation_stage = ZTM_PSU_HV_OP_WAIT_PGOOD;
    const uint32_t timeout_ms = pgood_timeout_ms == 0u
                                    ? ZTM_PSU_HV_DEFAULT_PGOOD_TIMEOUT_MS
                                    : pgood_timeout_ms;
    bool pgood = false;
    bool read_ok = false;
    uint32_t elapsed = 0u;
    for (elapsed = 0u; elapsed <= timeout_ms; elapsed += 5u) {
        read_ok = read_pgood_locked(device, &pgood);
        device->last_start_pgood_read_valid = read_ok;
        device->last_start_pgood = read_ok && pgood;

        if ((elapsed % 10u) == 0u || (read_ok && pgood)) {
            uint16_t adc_code = 0u;
            uint32_t drive_uV = 0u;
            if (read_drive_voltage_locked(device, &adc_code, &drive_uV)) {
                device->last_start_vsense_valid = true;
                device->last_start_vsense_adc_code = adc_code;
                device->last_start_drive_uV = drive_uV;
                if (drive_uV > device->last_start_peak_drive_uV) {
                    device->last_start_peak_drive_uV = drive_uV;
                }
            }
        }

        if (read_ok && pgood) {
            break;
        }
        if (elapsed < timeout_ms) {
            zap_delay_ms(device->config.time, 5u);
        }
    }

    device->last_start_wait_ms = elapsed > timeout_ms ? timeout_ms : elapsed;
    device->pgood_valid = read_ok;
    device->pgood = read_ok && pgood;
    if (!read_ok || !pgood) {
        (void)stop_locked(device);
        return set_operation_result(device, ZTM_PSU_HV_OP_WAIT_PGOOD, -6);
    }

    device->last_operation_stage = ZTM_PSU_HV_OP_READ_VSENSE;
    uint16_t adc_code = 0u;
    uint32_t drive_uV = 0u;
    if (!read_drive_voltage_locked(device, &adc_code, &drive_uV)) {
        (void)stop_locked(device);
        return set_operation_result(device, ZTM_PSU_HV_OP_READ_VSENSE, -7);
    }
    device->last_start_vsense_valid = true;
    device->last_start_vsense_adc_code = adc_code;
    device->last_start_drive_uV = drive_uV;
    if (drive_uV > device->last_start_peak_drive_uV) {
        device->last_start_peak_drive_uV = drive_uV;
    }

    device->last_operation_stage = ZTM_PSU_HV_OP_VALIDATE_E60_INPUT;
    if (!ztm_psu_hv_e60_input_within_rating(drive_uV)) {
        (void)stop_locked(device);
        return set_operation_result(device, ZTM_PSU_HV_OP_VALIDATE_E60_INPUT, -8);
    }

    return set_operation_result(device, ZTM_PSU_HV_OP_COMPLETE, 0);
}

int ztm_psu_hv_init(ztm_psu_hv_t *device, const ztm_psu_hv_config_t *config)
{
    if (device == NULL || !config_valid(config)) {
        return -1;
    }

    memset(device, 0, sizeof(*device));
    device->config = *config;
    device->config.address_straps &= 0x03u;
    device->i2c_address = AD5282_ADDR(device->config.address_straps);

    if (!lock_op(device)) {
        return init_failed(device, ZTM_PSU_HV_INIT_NOT_STARTED, -2);
    }

    device->last_init_stage = ZTM_PSU_HV_INIT_SAFE_CONTROLS;
    if (device->config.prepare(device->config.control_context) != 0 ||
        !enable_locked(device, false)) {
        unlock_op(device);
        return init_failed(device, ZTM_PSU_HV_INIT_SAFE_CONTROLS, -3);
    }

    device->last_init_stage = ZTM_PSU_HV_INIT_I2C;
    if (ad5282_init(&device->potentiometer,
                    device->config.i2c,
                    device->i2c_address) != AD5282_OK) {
        unlock_op(device);
        return init_failed(device, ZTM_PSU_HV_INIT_I2C, -4);
    }

    device->last_init_stage = ZTM_PSU_HV_INIT_AD5282_PROBE;
    if (ad5282_probe(&device->potentiometer) != AD5282_OK) {
        unlock_op(device);
        return init_failed(device, ZTM_PSU_HV_INIT_AD5282_PROBE, -5);
    }

    device->last_init_stage = ZTM_PSU_HV_INIT_SAFE_RDACS;
    if (ad5282_write_rdac(&device->potentiometer,
                          AD5282_RDAC1,
                          ZTM_PSU_HV_SAFE_RDAC_CODE,
                          false,
                          false) != AD5282_OK ||
        ad5282_write_rdac(&device->potentiometer,
                          AD5282_RDAC2,
                          ZTM_PSU_HV_SAFE_RDAC_CODE,
                          false,
                          false) != AD5282_OK) {
        unlock_op(device);
        return init_failed(device, ZTM_PSU_HV_INIT_SAFE_RDACS, -6);
    }

    device->last_init_stage = ZTM_PSU_HV_INIT_VERIFY_RDACS;
    if (!verify_rdacs_locked(device,
                             ZTM_PSU_HV_SAFE_RDAC_CODE,
                             ZTM_PSU_HV_SAFE_RDAC_CODE)) {
        unlock_op(device);
        return init_failed(device, ZTM_PSU_HV_INIT_VERIFY_RDACS, -7);
    }

    device->last_init_stage = ZTM_PSU_HV_INIT_STATUS;
    bool enabled = false;
    bool pgood = false;
    uint32_t drive_uV = 0u;
    if (!read_enable_locked(device, &enabled) || enabled ||
        !read_pgood_locked(device, &pgood) ||
        !read_drive_voltage_locked(device, NULL, &drive_uV)) {
        unlock_op(device);
        return init_failed(device, ZTM_PSU_HV_INIT_STATUS, -8);
    }

    device->enabled = false;
    device->pgood_valid = true;
    device->pgood = pgood;
    device->staged_code = ZTM_PSU_HV_SAFE_RDAC_CODE;
    device->ready = true;
    device->last_init_stage = ZTM_PSU_HV_INIT_READY;
    device->last_init_rc = 0;
    device->last_operation_stage = ZTM_PSU_HV_OP_COMPLETE;
    device->last_operation_rc = 0;
    unlock_op(device);

    emit(device, ZTM_PSU_HV_EVENT_READY, device->i2c_address);
    return 0;
}

int ztm_psu_hv_deinit(ztm_psu_hv_t *device)
{
    if (device == NULL || !config_valid(&device->config)) {
        return -1;
    }
    if (!lock_op(device)) {
        return -2;
    }
    const bool ok = stop_locked(device);
    device->ready = false;
    device->enabled = false;
    unlock_op(device);
    return ok ? 0 : -3;
}

bool ztm_psu_hv_ready(const ztm_psu_hv_t *device)
{
    return device != NULL && device->ready;
}

bool ztm_psu_hv_read_enable(ztm_psu_hv_t *device, bool *enabled)
{
    if (!device_bound(device) || enabled == NULL || !lock_op(device)) {
        return false;
    }
    const bool ok = read_enable_locked(device, enabled);
    if (ok) {
        device->enabled = *enabled;
    }
    unlock_op(device);
    return ok;
}

bool ztm_psu_hv_read_pgood(ztm_psu_hv_t *device, bool *pgood)
{
    if (!device_bound(device) || pgood == NULL || !lock_op(device)) {
        return false;
    }
    const bool ok = read_pgood_locked(device, pgood);
    device->pgood_valid = ok;
    if (ok) {
        device->pgood = *pgood;
    }
    unlock_op(device);
    return ok;
}

bool ztm_psu_hv_read_drive_voltage(ztm_psu_hv_t *device,
                                   uint16_t *adc_code,
                                   uint32_t *drive_uV)
{
    if (!device_bound(device) || drive_uV == NULL || !lock_op(device)) {
        return false;
    }
    const bool ok = read_drive_voltage_locked(device, adc_code, drive_uV);
    unlock_op(device);
    return ok;
}

bool ztm_psu_hv_read_rdac(ztm_psu_hv_t *device,
                          ad5282_channel_t channel,
                          uint8_t *code)
{
    if (!device_bound(device) || code == NULL || !lock_op(device)) {
        return false;
    }
    const bool ok = read_rdac_locked(device, channel, code);
    unlock_op(device);
    return ok;
}

bool ztm_psu_hv_stop(ztm_psu_hv_t *device)
{
    if (!device_bound(device) || !lock_op(device)) {
        return false;
    }
    const bool ok = stop_locked(device);
    unlock_op(device);
    return ok;
}

bool ztm_psu_hv_stage_code(ztm_psu_hv_t *device, uint8_t voltage_code)
{
    if (!device_bound(device) || !lock_op(device)) {
        return false;
    }
    const bool ok = stage_code_locked(device, voltage_code);
    unlock_op(device);
    return ok;
}

bool ztm_psu_hv_start(ztm_psu_hv_t *device,
                      uint8_t voltage_code,
                      uint32_t pgood_timeout_ms)
{
    if (!device_bound(device) || !lock_op(device)) {
        return false;
    }
    const bool ok = start_locked(device, voltage_code, pgood_timeout_ms);
    unlock_op(device);
    return ok;
}

bool ztm_psu_hv_read_snapshot(ztm_psu_hv_t *device, ztm_psu_hv_snapshot_t *snapshot)
{
    if (!device_bound(device) || snapshot == NULL || !lock_op(device)) {
        return false;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->enable_valid = read_enable_locked(device, &snapshot->enabled);
    snapshot->pgood_valid = read_pgood_locked(device, &snapshot->pgood);
    snapshot->rdac_readback_valid =
        read_rdac_locked(device, AD5282_RDAC1, &snapshot->rdac1_code) &&
        read_rdac_locked(device, AD5282_RDAC2, &snapshot->rdac2_code);
    snapshot->vsense_valid = read_drive_voltage_locked(device,
                                                        &snapshot->vsense_adc_code,
                                                        &snapshot->drive_uV);
    if (snapshot->vsense_valid) {
        snapshot->estimated_hv_mV = ztm_psu_hv_estimate_e60_output_mV(snapshot->drive_uV);
        snapshot->e60_input_overvoltage =
            !ztm_psu_hv_e60_input_within_rating(snapshot->drive_uV);
    }

    if (snapshot->enable_valid) {
        device->enabled = snapshot->enabled;
    }
    if (snapshot->pgood_valid) {
        device->pgood_valid = true;
        device->pgood = snapshot->pgood;
    }
    if (snapshot->rdac_readback_valid) {
        device->rdac_readback_valid = true;
        device->rdac1_code = snapshot->rdac1_code;
        device->rdac2_code = snapshot->rdac2_code;
    }

    const bool ok = snapshot->enable_valid && snapshot->pgood_valid &&
                    snapshot->rdac_readback_valid && snapshot->vsense_valid;
    unlock_op(device);
    return ok;
}

bool ztm_psu_hv_write_rdac_off(ztm_psu_hv_t *device,
                               ad5282_channel_t channel,
                               uint8_t code)
{
    if (!device_bound(device) || !device->ready || !lock_op(device)) {
        return false;
    }

    bool enabled = false;
    bool ok = read_enable_locked(device, &enabled) && !enabled &&
              ad5282_write_rdac(&device->potentiometer,
                                channel,
                                code,
                                false,
                                false) == AD5282_OK;
    uint8_t readback = 0u;
    if (ok) {
        ok = read_rdac_locked(device, channel, &readback) && readback == code;
    }
    if (ok) {
        if (channel == AD5282_RDAC1) {
            device->rdac1_code = code;
        } else if (channel == AD5282_RDAC2) {
            device->rdac2_code = code;
            device->staged_code = code;
        } else {
            ok = false;
        }
        device->rdac_readback_valid = ok;
    }

    unlock_op(device);
    return ok;
}

int ztm_psu_hv_probe(ztm_psu_hv_t *device, ztm_psu_hv_probe_result_t *result)
{
    if (device == NULL || result == NULL || !config_valid(&device->config)) {
        return -1;
    }
    if (!lock_op(device)) {
        return -2;
    }

    memset(result, 0, sizeof(*result));
    result->i2c_address = AD5282_ADDR(device->config.address_straps);
    result->controls_prepared = device->config.prepare(device->config.control_context) == 0;
    if (!result->controls_prepared) {
        unlock_op(device);
        return -3;
    }
    result->forced_off = enable_locked(device, false);
    if (!result->forced_off) {
        unlock_op(device);
        return -4;
    }

    if (ad5282_init(&device->potentiometer,
                    device->config.i2c,
                    result->i2c_address) != AD5282_OK) {
        unlock_op(device);
        return -5;
    }
    result->ad5282_present = ad5282_probe(&device->potentiometer) == AD5282_OK;
    if (result->ad5282_present) {
        result->rdac_readback_valid =
            read_rdac_locked(device, AD5282_RDAC1, &result->rdac1_code) &&
            read_rdac_locked(device, AD5282_RDAC2, &result->rdac2_code);
    }

    result->enable_valid = read_enable_locked(device, &result->enabled);
    result->pgood_valid = read_pgood_locked(device, &result->pgood);
    result->vsense_valid = read_drive_voltage_locked(device,
                                                      &result->vsense_adc_code,
                                                      &result->drive_uV);
    if (result->vsense_valid) {
        result->estimated_hv_mV = ztm_psu_hv_estimate_e60_output_mV(result->drive_uV);
        result->e60_input_overvoltage =
            !ztm_psu_hv_e60_input_within_rating(result->drive_uV);
    }

    const int rc = result->ad5282_present ? 0 : -6;
    unlock_op(device);
    return rc;
}

int ztm_psu_hv_poll(ztm_psu_hv_t *device)
{
    if (device == NULL || !device->ready) {
        return 0;
    }
    if (!try_lock_op(device)) {
        return 0;
    }

    device->poll_count++;
    bool enabled = false;
    bool pgood = false;
    const bool enable_ok = read_enable_locked(device, &enabled);
    const bool pgood_ok = read_pgood_locked(device, &pgood);
    device->enabled = enable_ok && enabled;
    device->pgood_valid = pgood_ok;
    device->pgood = pgood_ok && pgood;

    if (device->enabled && (!pgood_ok || !pgood)) {
        if (!device->pgood_fault_active) {
            device->pgood_fault_active = true;
            device->fault_events++;
            device->pgood_fault_events++;
            emit(device, ZTM_PSU_HV_EVENT_PGOOD_FAULT, pgood_ok ? 0u : 1u);
        }
        (void)stop_locked(device);
    } else {
        device->pgood_fault_active = false;
    }

    if (device->enabled) {
        uint32_t drive_uV = 0u;
        const bool vsense_ok = read_drive_voltage_locked(device, NULL, &drive_uV);
        if (!vsense_ok) {
            if (!device->vsense_fault_active) {
                device->vsense_fault_active = true;
                device->fault_events++;
                device->vsense_fault_events++;
                emit(device, ZTM_PSU_HV_EVENT_VSENSE_FAULT, 0u);
            }
            (void)stop_locked(device);
        } else {
            device->vsense_fault_active = false;
            if (!ztm_psu_hv_e60_input_within_rating(drive_uV)) {
                if (!device->e60_overvoltage_fault_active) {
                    device->e60_overvoltage_fault_active = true;
                    device->fault_events++;
                    device->e60_overvoltage_fault_events++;
                    emit(device, ZTM_PSU_HV_EVENT_E60_OVERVOLTAGE, drive_uV);
                }
                (void)stop_locked(device);
            } else {
                device->e60_overvoltage_fault_active = false;
            }
        }
    } else {
        device->vsense_fault_active = false;
        device->e60_overvoltage_fault_active = false;
    }

    unlock_op(device);
    return 0;
}
