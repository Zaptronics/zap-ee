// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zaptronics/drivers/mcp4901.h"
#include "zaptronics/platform/lock.h"
#include "zaptronics/platform/spi.h"
#include "zaptronics/platform/time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ztm_psu_tec_enable_set_fn)(void *context, bool enabled);
typedef int (*ztm_psu_tec_enable_get_fn)(void *context, bool *enabled);

typedef struct {
    const zap_spi_device_t *spi;
    const zap_time_t *time;
    const zap_lock_t *lock;
    void *control_context;
    ztm_psu_tec_enable_set_fn enable_set;
    ztm_psu_tec_enable_get_fn enable_get; /* optional */
    bool ref_buffered;
    bool gain_1x;
} ztm_psu_tec_config_t;

typedef struct {
    bool ready;
    bool enabled;
    bool enable_valid;
    bool dac_active;
    uint8_t dac_code;
    bool ref_buffered;
    bool gain_1x;
} ztm_psu_tec_status_t;

typedef struct {
    mcp4901_t dac;
    ztm_psu_tec_config_t config;
    bool ready;
    bool enabled;
    bool enable_valid;
    bool dac_active;
    uint8_t dac_code;
} ztm_psu_tec_t;

/* Initialise one explicit TEC PSU instance and place both controls in OFF state. */
int ztm_psu_tec_init(ztm_psu_tec_t *device, const ztm_psu_tec_config_t *config);
int ztm_psu_tec_deinit(ztm_psu_tec_t *device);
bool ztm_psu_tec_ready(const ztm_psu_tec_t *device);

/* Low-level board operations for applications coordinating several TEC rails. */
bool ztm_psu_tec_set_enable(ztm_psu_tec_t *device, bool enabled);
bool ztm_psu_tec_set_raw(ztm_psu_tec_t *device, uint8_t code);
bool ztm_psu_tec_shutdown_dac(ztm_psu_tec_t *device);
bool ztm_psu_tec_set_millivolts(ztm_psu_tec_t *device,
                                uint32_t vref_mV,
                                uint32_t vout_mV,
                                uint8_t *written_code);

/* Safe operational helpers: EN low is authoritative; DAC shutdown is secondary. */
bool ztm_psu_tec_start(ztm_psu_tec_t *device, uint8_t code, uint32_t settle_us);
bool ztm_psu_tec_stop(ztm_psu_tec_t *device);
bool ztm_psu_tec_get_status(ztm_psu_tec_t *device, ztm_psu_tec_status_t *status);

#ifdef __cplusplus
}
#endif
