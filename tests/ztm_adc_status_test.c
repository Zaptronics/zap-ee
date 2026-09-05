// SPDX-License-Identifier: Apache-2.0
#include <stdint.h>

#include "zaptronics/modules/ztm_adc.h"

typedef struct {
    unsigned acquire_count;
    unsigned release_count;
    uint32_t last_timeout_us;
} lock_probe_t;

static int test_lock(void *context, uint32_t timeout_us)
{
    lock_probe_t *probe = (lock_probe_t *)context;
    probe->acquire_count++;
    probe->last_timeout_us = timeout_us;
    return 0;
}

static void test_unlock(void *context)
{
    lock_probe_t *probe = (lock_probe_t *)context;
    probe->release_count++;
}

int main(void)
{
    ztm_adc_status_t null_status;
    ztm_adc_rtd_sample_t null_rtd;
    ztm_adc_tc_sample_t null_tc;
    thermocouple_type_t null_type;
    uint8_t null_adc_status = 0u;
    uint32_t null_error_mask = 0u;
    uint32_t null_register = 0u;
    ztm_adc_measure_diag_t null_diag;

    if (ztm_adc_ready(NULL) ||
        ztm_adc_get_status(NULL, &null_status) ||
        ztm_adc_read_cold_junction(NULL, &null_rtd) ||
        ztm_adc_read_rtd(NULL, 1u, &null_rtd) ||
        ztm_adc_set_thermocouple_type(NULL, 1u, THERMOCOUPLE_TYPE_K) ||
        ztm_adc_get_thermocouple_type(NULL, 1u, &null_type) ||
        ztm_adc_read_thermocouple(NULL, 1u, &null_tc) ||
        ztm_adc_read_error_status(NULL, &null_adc_status, &null_error_mask) ||
        ztm_adc_verify_configuration(NULL) ||
        ztm_adc_read_register(NULL, 0u, 1u, &null_register) ||
        ztm_adc_calibrate_all(NULL) ||
        ztm_adc_iout_off(NULL) ||
        ztm_adc_get_last_measurement(NULL, &null_diag)) {
        return 1;
    }

    lock_probe_t probe = {0};
    const zap_lock_t lock = {
        .context = &probe,
        .acquire = test_lock,
        .release = test_unlock,
        .timeout_us = 1234u,
    };

    ztm_adc_t adc = {0};
    adc.config.lock = &lock;
    adc.ready = true;
    adc.device_id = 0x14u;
    adc.configuration_verified = true;
    adc.por_fault_active = true;
    adc.reference_resistor_ohms = 1999.5;
    adc.rtd_excitation = AD7124_IOUT_500UA;
    adc.thermocouple_types[0] = THERMOCOUPLE_TYPE_K;
    adc.thermocouple_types[1] = THERMOCOUPLE_TYPE_J;
    adc.thermocouple_types[2] = THERMOCOUPLE_TYPE_T;
    adc.last_error_mask = 0x123456u;
    adc.error_event_count = 3u;
    adc.communication_error_count = 4u;
    adc.configuration_error_count = 5u;
    adc.last_event_valid = true;
    adc.last_event.type = ZTM_ADC_EVENT_ADC_ERROR;
    adc.last_event.error_mask = 0x20u;
    adc.last_measurement.stage = ZTM_ADC_MEASURE_OK;
    adc.last_measurement.raw_code = 0xABCDEFu;

    ztm_adc_status_t status;
    if (!ztm_adc_get_status(&adc, &status)) {
        return 2;
    }
    if (probe.acquire_count != 1u || probe.release_count != 1u ||
        probe.last_timeout_us != 1234u) {
        return 3;
    }
    if (!status.ready || status.device_id != 0x14u ||
        !status.configuration_verified || !status.por_fault_active) {
        return 4;
    }
    if (status.reference_resistor_ohms != 1999.5 ||
        status.rtd_excitation != AD7124_IOUT_500UA) {
        return 5;
    }
    if (status.thermocouple_types[0] != THERMOCOUPLE_TYPE_K ||
        status.thermocouple_types[1] != THERMOCOUPLE_TYPE_J ||
        status.thermocouple_types[2] != THERMOCOUPLE_TYPE_T) {
        return 6;
    }
    if (status.last_error_mask != 0x123456u ||
        status.error_event_count != 3u ||
        status.communication_error_count != 4u ||
        status.configuration_error_count != 5u) {
        return 7;
    }
    if (!status.last_event_valid || status.last_event.type != ZTM_ADC_EVENT_ADC_ERROR ||
        status.last_event.error_mask != 0x20u) {
        return 8;
    }
    if (status.last_measurement.stage != ZTM_ADC_MEASURE_OK ||
        status.last_measurement.raw_code != 0xABCDEFu) {
        return 9;
    }

    return 0;
}
