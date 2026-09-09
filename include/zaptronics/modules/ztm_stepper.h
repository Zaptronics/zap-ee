// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zaptronics/drivers/l6470.h"
#include "zaptronics/platform/framed_spi.h"
#include "zaptronics/platform/lock.h"
#include "zaptronics/platform/time.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZTM_STEPPER_DRIVER_COUNT 4u
#define ZTM_STEPPER_ACTIVE_DRIVER_COUNT 3u
#define ZTM_STEPPER_UNUSED_DRIVER_ID 4u
#define ZTM_STEPPER_OPERATION_TIMEOUT_US 5000u

#define ZTM_STEPPER_DEFAULT_KVAL_HOLD 0u
#define ZTM_STEPPER_DEFAULT_KVAL_RUN 12u
#define ZTM_STEPPER_DEFAULT_KVAL_ACC 12u
#define ZTM_STEPPER_DEFAULT_KVAL_DEC 12u
#define ZTM_STEPPER_DEFAULT_ACCEL_SPS2 25.0f
#define ZTM_STEPPER_DEFAULT_DECEL_SPS2 25.0f
#define ZTM_STEPPER_DEFAULT_MAX_SPEED_SPS 500.0f

typedef enum {
    ZTM_STEPPER_MICROSTEP_FULL = 0,
    ZTM_STEPPER_MICROSTEP_HALF = 1,
    ZTM_STEPPER_MICROSTEP_QUARTER = 2,
    ZTM_STEPPER_MICROSTEP_EIGHTH = 3,
    ZTM_STEPPER_MICROSTEP_SIXTEENTH = 4,
    ZTM_STEPPER_MICROSTEP_THIRTY_SECOND = 5,
    ZTM_STEPPER_MICROSTEP_SIXTY_FOURTH = 6,
    ZTM_STEPPER_MICROSTEP_ONE_TWENTY_EIGHTH = 7,
} ztm_stepper_microstep_t;

#define ZTM_STEPPER_DEFAULT_MICROSTEP ZTM_STEPPER_MICROSTEP_QUARTER

typedef struct {
    uint8_t hold;
    uint8_t run;
    uint8_t acc;
    uint8_t dec;
} ztm_stepper_kval_config_t;

typedef enum {
    ZTM_STEPPER_EVENT_STALL_ASSERTED = 0,
    ZTM_STEPPER_EVENT_STALL_CLEARED,
    ZTM_STEPPER_EVENT_THERMAL_WARNING_ASSERTED,
    ZTM_STEPPER_EVENT_THERMAL_WARNING_CLEARED,
    ZTM_STEPPER_EVENT_THERMAL_SHUTDOWN_ASSERTED,
    ZTM_STEPPER_EVENT_THERMAL_SHUTDOWN_CLEARED,
    ZTM_STEPPER_EVENT_COMMUNICATION_ERROR,
} ztm_stepper_event_type_t;

typedef struct {
    ztm_stepper_event_type_t type;
    uint8_t driver_1based;
    uint16_t status;
} ztm_stepper_event_t;

typedef void (*ztm_stepper_event_fn)(void *context, const ztm_stepper_event_t *event);
typedef int (*ztm_stepper_power_set_fn)(void *context, bool enabled);

typedef struct {
    const zap_spi_framed_device_t *spi;
    const zap_time_t *time;
    const zap_lock_t *lock;
    void *control_context;
    ztm_stepper_power_set_fn power_set;
    void *event_context;
    ztm_stepper_event_fn event;
} ztm_stepper_config_t;

typedef struct {
    ztm_stepper_config_t config;
    ztm_stepper_kval_config_t kval_config[ZTM_STEPPER_DRIVER_COUNT];
    ztm_stepper_microstep_t microstep_mode[ZTM_STEPPER_DRIVER_COUNT];
    uint16_t last_status[ZTM_STEPPER_DRIVER_COUNT];
    bool stall[ZTM_STEPPER_DRIVER_COUNT];
    bool thermal_warning[ZTM_STEPPER_DRIVER_COUNT];
    bool thermal_shutdown[ZTM_STEPPER_DRIVER_COUNT];
    bool ready;
} ztm_stepper_t;

typedef struct {
    bool ready;
    ztm_stepper_kval_config_t kval_config[ZTM_STEPPER_DRIVER_COUNT];
    ztm_stepper_microstep_t microstep_mode[ZTM_STEPPER_DRIVER_COUNT];
    uint16_t last_status[ZTM_STEPPER_DRIVER_COUNT];
    bool stall[ZTM_STEPPER_DRIVER_COUNT];
    bool thermal_warning[ZTM_STEPPER_DRIVER_COUNT];
    bool thermal_shutdown[ZTM_STEPPER_DRIVER_COUNT];
} ztm_stepper_status_t;

int ztm_stepper_init(ztm_stepper_t *module, const ztm_stepper_config_t *config);
int ztm_stepper_deinit(ztm_stepper_t *module);
int ztm_stepper_poll(ztm_stepper_t *module);
bool ztm_stepper_ready(const ztm_stepper_t *module);
void ztm_stepper_get_snapshot(const ztm_stepper_t *module, ztm_stepper_status_t *snapshot);

bool ztm_stepper_nop(ztm_stepper_t *module, uint8_t responses[ZTM_STEPPER_DRIVER_COUNT]);
bool ztm_stepper_get_status(ztm_stepper_t *module, uint8_t device_id, uint16_t *status);
bool ztm_stepper_get_param(ztm_stepper_t *module, uint8_t device_id, uint8_t reg,
                           uint8_t nbytes, uint32_t *value);
bool ztm_stepper_set_param(ztm_stepper_t *module, uint8_t device_id, uint8_t reg,
                           uint8_t nbytes, uint32_t value);
bool ztm_stepper_get_kval_config(ztm_stepper_t *module, uint8_t device_id,
                                 ztm_stepper_kval_config_t *config);
bool ztm_stepper_set_kval_config(ztm_stepper_t *module, uint8_t device_id,
                                 const ztm_stepper_kval_config_t *config);
bool ztm_stepper_set_kval_config_all(ztm_stepper_t *module,
                                     const ztm_stepper_kval_config_t *config);
bool ztm_stepper_get_microstep(ztm_stepper_t *module, uint8_t device_id,
                               ztm_stepper_microstep_t *mode);
bool ztm_stepper_set_microstep(ztm_stepper_t *module, uint8_t device_id,
                               ztm_stepper_microstep_t mode);
bool ztm_stepper_set_microstep_all(ztm_stepper_t *module, ztm_stepper_microstep_t mode);
bool ztm_stepper_reset(ztm_stepper_t *module, uint8_t device_id);
bool ztm_stepper_reset_all(ztm_stepper_t *module);
bool ztm_stepper_hard_hiz(ztm_stepper_t *module, uint8_t device_id);
bool ztm_stepper_hard_hiz_all(ztm_stepper_t *module);
bool ztm_stepper_soft_stop(ztm_stepper_t *module, uint8_t device_id);
bool ztm_stepper_soft_stop_all(ztm_stepper_t *module);
bool ztm_stepper_run(ztm_stepper_t *module, uint8_t device_id, bool dir_fwd, float steps_per_s);
bool ztm_stepper_run_all(ztm_stepper_t *module, bool dir_fwd, float steps_per_s);
bool ztm_stepper_move(ztm_stepper_t *module, uint8_t device_id, bool dir_fwd, uint32_t steps);
bool ztm_stepper_apply_test_config(ztm_stepper_t *module, uint8_t device_id);
bool ztm_stepper_apply_test_config_all(ztm_stepper_t *module);

/* Exposed for deterministic host tests and diagnostics. Logical driver 1 is
 * closest to the controller, so its byte is last on the wire. */
bool ztm_stepper_compose_frame(uint8_t device_id, uint8_t value,
                               uint8_t wire_frame[ZTM_STEPPER_DRIVER_COUNT]);

#ifdef __cplusplus
}
#endif
