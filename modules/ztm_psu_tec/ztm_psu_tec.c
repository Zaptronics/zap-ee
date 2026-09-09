// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/modules/ztm_psu_tec.h"

#include <string.h>

static bool config_valid(const ztm_psu_tec_config_t *config)
{
    return config != NULL &&
           zap_spi_device_valid(config->spi) &&
           zap_time_valid(config->time) &&
           zap_lock_valid(config->lock) &&
           config->enable_set != NULL;
}

static bool lock_op(ztm_psu_tec_t *device)
{
    return device != NULL && zap_lock_acquire(device->config.lock) == 0;
}

static void unlock_op(ztm_psu_tec_t *device)
{
    if (device != NULL) {
        zap_lock_release(device->config.lock);
    }
}

static bool set_enable_locked(ztm_psu_tec_t *device, bool enabled)
{
    if (device == NULL || device->config.enable_set == NULL ||
        device->config.enable_set(device->config.control_context, enabled) != 0) {
        if (device != NULL) {
            device->enable_valid = false;
        }
        return false;
    }
    device->enabled = enabled;
    device->enable_valid = true;
    return true;
}

static bool set_raw_locked(ztm_psu_tec_t *device, uint8_t code)
{
    if (device == NULL || mcp4901_set_raw(&device->dac, code) != MCP4901_OK) {
        return false;
    }
    device->dac_code = code;
    device->dac_active = true;
    return true;
}

static bool shutdown_locked(ztm_psu_tec_t *device)
{
    if (device == NULL || mcp4901_shutdown(&device->dac) != MCP4901_OK) {
        return false;
    }
    device->dac_code = 0u;
    device->dac_active = false;
    return true;
}

static bool stop_locked(ztm_psu_tec_t *device)
{
    if (device == NULL) {
        return false;
    }

    /* EN low is authoritative. Attempt DAC shutdown even if EN control failed. */
    const bool disabled = set_enable_locked(device, false);
    const bool shutdown = shutdown_locked(device);
    return disabled && shutdown;
}

int ztm_psu_tec_init(ztm_psu_tec_t *device, const ztm_psu_tec_config_t *config)
{
    if (device == NULL || !config_valid(config)) {
        return -1;
    }

    memset(device, 0, sizeof(*device));
    device->config = *config;
    if (mcp4901_init(&device->dac,
                     config->spi,
                     config->ref_buffered,
                     config->gain_1x) != MCP4901_OK) {
        return -2;
    }

    if (!lock_op(device)) {
        return -3;
    }
    const bool safe = stop_locked(device);
    unlock_op(device);
    if (!safe) {
        return -4;
    }

    device->ready = true;
    return 0;
}

int ztm_psu_tec_deinit(ztm_psu_tec_t *device)
{
    if (device == NULL) {
        return -1;
    }
    if (!lock_op(device)) {
        return -2;
    }
    const bool ok = stop_locked(device);
    device->ready = false;
    unlock_op(device);
    return ok ? 0 : -3;
}

bool ztm_psu_tec_ready(const ztm_psu_tec_t *device)
{
    return device != NULL && device->ready;
}

bool ztm_psu_tec_set_enable(ztm_psu_tec_t *device, bool enabled)
{
    if (!ztm_psu_tec_ready(device) || !lock_op(device)) {
        return false;
    }
    const bool ok = set_enable_locked(device, enabled);
    unlock_op(device);
    return ok;
}

bool ztm_psu_tec_set_raw(ztm_psu_tec_t *device, uint8_t code)
{
    if (!ztm_psu_tec_ready(device) || !lock_op(device)) {
        return false;
    }
    const bool ok = set_raw_locked(device, code);
    unlock_op(device);
    return ok;
}

bool ztm_psu_tec_shutdown_dac(ztm_psu_tec_t *device)
{
    if (!ztm_psu_tec_ready(device) || !lock_op(device)) {
        return false;
    }
    const bool ok = shutdown_locked(device);
    unlock_op(device);
    return ok;
}

bool ztm_psu_tec_set_millivolts(ztm_psu_tec_t *device,
                                uint32_t vref_mV,
                                uint32_t vout_mV,
                                uint8_t *written_code)
{
    if (!ztm_psu_tec_ready(device) || !lock_op(device)) {
        return false;
    }

    uint8_t code = 0u;
    const bool ok = mcp4901_set_millivolts(&device->dac,
                                           vref_mV,
                                           vout_mV,
                                           &code) == MCP4901_OK;
    if (ok) {
        device->dac_code = code;
        device->dac_active = true;
        if (written_code != NULL) {
            *written_code = code;
        }
    }
    unlock_op(device);
    return ok;
}

bool ztm_psu_tec_start(ztm_psu_tec_t *device, uint8_t code, uint32_t settle_us)
{
    if (!ztm_psu_tec_ready(device) || !lock_op(device)) {
        return false;
    }

    bool ok = set_enable_locked(device, false) &&
              set_raw_locked(device, code) &&
              set_enable_locked(device, true);
    if (ok && settle_us != 0u) {
        zap_delay_us(device->config.time, settle_us);
    }
    if (!ok) {
        (void)stop_locked(device);
    }

    unlock_op(device);
    return ok;
}

bool ztm_psu_tec_stop(ztm_psu_tec_t *device)
{
    if (!ztm_psu_tec_ready(device) || !lock_op(device)) {
        return false;
    }
    const bool ok = stop_locked(device);
    unlock_op(device);
    return ok;
}

bool ztm_psu_tec_get_status(ztm_psu_tec_t *device, ztm_psu_tec_status_t *status)
{
    if (device == NULL || status == NULL || !lock_op(device)) {
        return false;
    }

    if (device->config.enable_get != NULL) {
        bool enabled = false;
        if (device->config.enable_get(device->config.control_context, &enabled) == 0) {
            device->enabled = enabled;
            device->enable_valid = true;
        } else {
            device->enable_valid = false;
        }
    }

    *status = (ztm_psu_tec_status_t) {
        .ready = device->ready,
        .enabled = device->enabled,
        .enable_valid = device->enable_valid,
        .dac_active = device->dac_active,
        .dac_code = device->dac_code,
        .ref_buffered = device->dac.ref_buffered,
        .gain_1x = device->dac.gain_1x,
    };
    unlock_op(device);
    return true;
}
