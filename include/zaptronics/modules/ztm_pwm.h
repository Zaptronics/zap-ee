// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zaptronics/drivers/emc2303.h"
#include "zaptronics/drivers/pac195x.h"
#include "zaptronics/drivers/sx1509.h"
#include "zaptronics/platform/i2c.h"
#include "zaptronics/platform/time.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZTM_PWM_CHANNEL_COUNT 12u
#define ZTM_PWM_FAN_COUNT 6u
#define ZTM_PWM_GPIO_COUNT 4u
#define ZTM_PWM_PAC_COUNT 3u
#define ZTM_PWM_EMC_COUNT 2u

#define ZTM_PWM_SX1509_ADDR SX1509_I2C_ADDR_3
#define ZTM_PWM_PAC1_ADDR 0x10u
#define ZTM_PWM_PAC2_ADDR 0x11u
#define ZTM_PWM_PAC3_ADDR 0x12u
#define ZTM_PWM_EMC1_ADDR 0x2eu
#define ZTM_PWM_EMC2_ADDR 0x2fu
#define ZTM_PWM_SHUNT_UOHM 50000u
#define ZTM_PWM_POWER_SAFE_TIMEOUT_MS 5000u
#define ZTM_PWM_ERR_POWER_SAFE_TIMEOUT (-40)
#define ZTM_PWM_ERR_POWER_SAFE_READ (-41)
#define ZTM_PWM_ERR_POWER_SAFE_LOW (-42)

#ifndef ZTM_PWM_FAN_DEFAULT_PUSH_PULL_MASK
#define ZTM_PWM_FAN_DEFAULT_PUSH_PULL_MASK 0x3fu
#endif

typedef enum {
    ZTM_PWM_PAC_TOPOLOGY_NONE = 0,
    ZTM_PWM_PAC_TOPOLOGY_NATIVE_4CH,
    ZTM_PWM_PAC_TOPOLOGY_PAC1952_2_SALVAGE,
    ZTM_PWM_PAC_TOPOLOGY_UNSUPPORTED,
} ztm_pwm_pac_topology_t;

typedef enum {
    ZTM_PWM_CURRENT_SOURCE_NONE = 0,
    ZTM_PWM_CURRENT_SOURCE_VSENSE,
    ZTM_PWM_CURRENT_SOURCE_VBUS,
} ztm_pwm_current_source_t;

typedef struct {
    uint8_t pac_1based;
    uint8_t input_1based;
    ztm_pwm_current_source_t source;
    uint16_t raw;
    int32_t shunt_uV;
    int32_t current_uA;
    bool coarse_resolution;
} ztm_pwm_current_reading_t;

typedef enum {
    ZTM_PWM_INIT_NONE = 0,
    ZTM_PWM_INIT_POWER_OFF,
    ZTM_PWM_INIT_POWER_ON,
    ZTM_PWM_INIT_SX1509,
    ZTM_PWM_INIT_LOW_SIDE_OUTPUTS,
    ZTM_PWM_INIT_USER_GPIOS,
    ZTM_PWM_INIT_PAC1,
    ZTM_PWM_INIT_PAC2,
    ZTM_PWM_INIT_PAC3,
    ZTM_PWM_INIT_EMC1,
    ZTM_PWM_INIT_EMC2,
    ZTM_PWM_INIT_FAN_SAFE_STATE,
    ZTM_PWM_INIT_FAN_OUTPUT_CONFIG,
    ZTM_PWM_INIT_FAN_INTERRUPTS,
    ZTM_PWM_INIT_CLEAR_STATUS,
    ZTM_PWM_INIT_READY,
    ZTM_PWM_INIT_POWER_SAFE,
} ztm_pwm_init_stage_t;

typedef struct {
    int power_safe_rc; /* 0: high verified; -40 timeout, -41 read failure, -42 lost. */
    bool power_control_ok;
    bool power_was_enabled;
    bool power_temporarily_enabled;
    bool power_restored_off;
    bool enable_read_ok;
    bool enable_high;
    bool irq_read_ok;
    bool irq_asserted;
    bool sx_present;
    uint8_t sx_dir_a;
    bool pac_present[ZTM_PWM_PAC_COUNT];
    uint8_t pac_product_id[ZTM_PWM_PAC_COUNT];
    uint8_t pac_manufacturer_id[ZTM_PWM_PAC_COUNT];
    uint8_t pac_revision_id[ZTM_PWM_PAC_COUNT];
    uint8_t pac_channels[ZTM_PWM_PAC_COUNT];
    bool emc_present[ZTM_PWM_EMC_COUNT];
    uint8_t emc_product_id[ZTM_PWM_EMC_COUNT];
    uint8_t emc_manufacturer_id[ZTM_PWM_EMC_COUNT];
    uint8_t emc_revision_id[ZTM_PWM_EMC_COUNT];
    uint8_t devices_found;
} ztm_pwm_probe_result_t;

typedef struct {
    int power_safe_rc; /* 0: high verified; -40 timeout, -41 read failure, -42 lost. */
    bool power_control_ok;
    bool power_was_enabled;
    bool power_temporarily_enabled;
    bool power_restored_off;
    bool address_present[128];
    uint8_t devices_found;
} ztm_pwm_i2c_scan_result_t;

typedef enum {
    ZTM_PWM_FAN_OUTPUT_OPEN_DRAIN = 0,
    ZTM_PWM_FAN_OUTPUT_PUSH_PULL,
} ztm_pwm_fan_output_mode_t;

typedef enum {
    ZTM_PWM_FAN_TACH_OK = 0,
    ZTM_PWM_FAN_TACH_STALL,
    ZTM_PWM_FAN_TACH_READ_ERROR,
} ztm_pwm_fan_tach_status_t;

typedef enum {
    ZTM_PWM_GPIO_INPUT = 0,
    ZTM_PWM_GPIO_OUTPUT = 1,
} ztm_pwm_gpio_mode_t;

typedef enum {
    ZTM_PWM_GPIO_PULL_NONE = 0,
    ZTM_PWM_GPIO_PULL_UP,
    ZTM_PWM_GPIO_PULL_DOWN,
} ztm_pwm_gpio_pull_t;

typedef enum {
    ZTM_PWM_EVENT_SX1509_INTERRUPT = 0,
    ZTM_PWM_EVENT_PAC_ALERT,
    ZTM_PWM_EVENT_FAN_WATCHDOG,
    ZTM_PWM_EVENT_FAN_SPINUP_FAILURE,
    ZTM_PWM_EVENT_FAN_DRIVE_FAILURE,
    ZTM_PWM_EVENT_IRQ_UNRESOLVED,
    ZTM_PWM_EVENT_POWER_SAFE_ERROR, /* detail is the positive magnitude of rc. */
} ztm_pwm_event_type_t;

typedef struct {
    ztm_pwm_event_type_t type;
    uint8_t device_1based;
    uint8_t channel_1based;
    uint32_t detail;
} ztm_pwm_event_t;

typedef int (*ztm_pwm_power_set_fn)(void *context, bool enabled);
typedef int (*ztm_pwm_power_get_fn)(void *context, bool *enabled);
typedef int (*ztm_pwm_irq_read_fn)(void *context, bool *asserted);
typedef void (*ztm_pwm_event_fn)(void *context, const ztm_pwm_event_t *event);

/** Platform/application services required by one ZTM_PWM instance.
 *
 * i2c, time, power_set and power_safe_read are required. power_get, irq_read and event are
 * optional. control_context is passed to power/IRQ callbacks; event_context is
 * passed independently to the structured event callback.
 */
typedef struct {
    const zap_i2c_bus_t *i2c;
    const zap_time_t *time;
    void *control_context;
    ztm_pwm_power_set_fn power_set;
    ztm_pwm_power_get_fn power_get;
    ztm_pwm_irq_read_fn irq_read;
    void *event_context;
    ztm_pwm_event_fn event;
    /* Active-high POWER_SAFE, using control_context; 0 return means valid read. */
    ztm_pwm_power_get_fn power_safe_read;
    uint32_t power_safe_timeout_ms; /* Zero selects the 5000 ms default. */
} ztm_pwm_config_t;

typedef struct {
    emc2303_t fan_controller[ZTM_PWM_EMC_COUNT];
    pac195x_t current_monitor[ZTM_PWM_PAC_COUNT];
    bool current_monitor_ready[ZTM_PWM_PAC_COUNT];
    int current_monitor_rc[ZTM_PWM_PAC_COUNT];
    ztm_pwm_pac_topology_t current_monitor_topology[ZTM_PWM_PAC_COUNT];
    sx1509_t output_mux;

    ztm_pwm_config_t config;
    bool ready;
    int power_safe_rc; /* Latched until an explicit startup/probe/scan. */
    ztm_pwm_init_stage_t last_init_stage;
    int last_init_rc;

    uint8_t channel_duty_pct[ZTM_PWM_CHANNEL_COUNT];
    uint8_t fan_duty_pct[ZTM_PWM_FAN_COUNT];
    bool fan_closed_loop[ZTM_PWM_FAN_COUNT];
    uint32_t fan_target_rpm[ZTM_PWM_FAN_COUNT];
    ztm_pwm_gpio_mode_t gpio_mode[ZTM_PWM_GPIO_COUNT];
    ztm_pwm_gpio_pull_t gpio_pull[ZTM_PWM_GPIO_COUNT];
    sx1509_sense_t gpio_irq_sense[ZTM_PWM_GPIO_COUNT];
    bool gpio_level[ZTM_PWM_GPIO_COUNT];

    uint32_t irq_count;
    uint32_t irq_scan_errors;
    bool irq_unresolved_reported;
} ztm_pwm_t;

/** Initialise one explicit ZTM_PWM instance and place the board in a safe, ready state. */
int ztm_pwm_init(ztm_pwm_t *device, const ztm_pwm_config_t *config);
/** Scan/clear module IRQ sources and emit configured structured events. */
int ztm_pwm_poll(ztm_pwm_t *device);
/** Stop outputs, disable module power, and mark the instance not ready. */
int ztm_pwm_deinit(ztm_pwm_t *device);
bool ztm_pwm_ready(const ztm_pwm_t *device);
/* Check readiness and live POWER_SAFE before direct diagnostic driver access. */
bool ztm_pwm_operational(ztm_pwm_t *device);
bool ztm_pwm_power_off(ztm_pwm_t *device);
bool ztm_pwm_reinitialize(ztm_pwm_t *device);
bool ztm_pwm_read_enable(const ztm_pwm_t *device, bool *enabled);
bool ztm_pwm_read_irq_asserted(const ztm_pwm_t *device, bool *asserted);
const char *ztm_pwm_init_stage_name(ztm_pwm_init_stage_t stage);
int ztm_pwm_probe_devices(ztm_pwm_t *device, ztm_pwm_probe_result_t *result, bool ensure_power_on);
int ztm_pwm_i2c_scan(ztm_pwm_t *device, ztm_pwm_i2c_scan_result_t *result, bool ensure_power_on);
const char *ztm_pwm_pac_topology_name(ztm_pwm_pac_topology_t topology);
const char *ztm_pwm_current_source_name(ztm_pwm_current_source_t source);

/* Low-side PWM outputs: channels are 1..12 and duty is integer percent 0..100. */
bool ztm_pwm_channel_set(ztm_pwm_t *device, uint8_t channel_1based, uint8_t duty_pct);
bool ztm_pwm_channel_set_all(ztm_pwm_t *device, uint8_t duty_pct);
bool ztm_pwm_channel_stop(ztm_pwm_t *device, uint8_t channel_1based);
/* False if unavailable or any zero-duty write fails; does not confirm physical fan stop. */
bool ztm_pwm_stop_all(ztm_pwm_t *device);
uint8_t ztm_pwm_channel_get_duty(const ztm_pwm_t *device, uint8_t channel_1based);
uint8_t ztm_pwm_channel_get_sx_pin(uint8_t channel_1based);

/* Per-channel current measurement through the board's PAC195x routing. */
bool ztm_pwm_read_channel(ztm_pwm_t *device, uint8_t channel_1based, bool averaged, ztm_pwm_current_reading_t *reading);
bool ztm_pwm_read_current_uA(ztm_pwm_t *device, uint8_t channel_1based, bool averaged, int32_t *current_uA);
bool ztm_pwm_read_all_currents_uA(ztm_pwm_t *device, bool averaged, int32_t current_uA[ZTM_PWM_CHANNEL_COUNT]);
bool ztm_pwm_channel_get_current_route(const ztm_pwm_t *device, uint8_t channel_1based, uint8_t *pac_1based, ztm_pwm_current_source_t *source, uint8_t *input_1based);

/* Fan control/tachometer API: fans are numbered 1..6. */
bool ztm_pwm_fan_set(ztm_pwm_t *device, uint8_t fan_1based, uint8_t duty_pct);
bool ztm_pwm_fan_set_raw(ztm_pwm_t *device, uint8_t fan_1based, uint8_t raw);
bool ztm_pwm_fan_set_target_rpm(ztm_pwm_t *device, uint8_t fan_1based, uint32_t rpm);
bool ztm_pwm_fan_set_open_loop(ztm_pwm_t *device, uint8_t fan_1based, uint8_t duty_pct);
bool ztm_pwm_fan_set_all(ztm_pwm_t *device, uint8_t duty_pct);
bool ztm_pwm_fan_set_output_mode(ztm_pwm_t *device, uint8_t fan_1based, ztm_pwm_fan_output_mode_t mode);
bool ztm_pwm_fan_set_output_mode_all(ztm_pwm_t *device, ztm_pwm_fan_output_mode_t mode);
bool ztm_pwm_fan_get_output_mode(ztm_pwm_t *device, uint8_t fan_1based, ztm_pwm_fan_output_mode_t *mode);
const char *ztm_pwm_fan_output_mode_name(ztm_pwm_fan_output_mode_t mode);
bool ztm_pwm_fan_stop(ztm_pwm_t *device, uint8_t fan_1based);
bool ztm_pwm_fan_stop_all(ztm_pwm_t *device);
bool ztm_pwm_fan_get_rpm(ztm_pwm_t *device, uint8_t fan_1based, uint32_t *rpm);
bool ztm_pwm_fan_get_tach(ztm_pwm_t *device, uint8_t fan_1based, uint16_t *tach_count, uint32_t *rpm);
ztm_pwm_fan_tach_status_t ztm_pwm_fan_get_tach_status(ztm_pwm_t *device, uint8_t fan_1based, uint16_t *tach_count, uint32_t *rpm);
bool ztm_pwm_fan_get_route(uint8_t fan_1based, uint8_t *emc_1based, uint8_t *local_fan_1based);
uint8_t ztm_pwm_fan_get_duty(const ztm_pwm_t *device, uint8_t fan_1based);

/* General-purpose module GPIO API: GPIOs are numbered 1..4. */
bool ztm_pwm_gpio_configure(ztm_pwm_t *device, uint8_t gpio_1based, ztm_pwm_gpio_mode_t mode);
bool ztm_pwm_gpio_configure_input(ztm_pwm_t *device, uint8_t gpio_1based, ztm_pwm_gpio_pull_t pull);
bool ztm_pwm_gpio_configure_irq(ztm_pwm_t *device, uint8_t gpio_1based, sx1509_sense_t sense);
bool ztm_pwm_gpio_toggle(ztm_pwm_t *device, uint8_t gpio_1based);
bool ztm_pwm_gpio_write(ztm_pwm_t *device, uint8_t gpio_1based, bool level);
bool ztm_pwm_gpio_read(ztm_pwm_t *device, uint8_t gpio_1based, bool *level);
uint8_t ztm_pwm_gpio_get_sx_pin(uint8_t gpio_1based);

int ztm_pwm_scan_irq_sources(ztm_pwm_t *device, bool force_scan);
bool ztm_pwm_test(ztm_pwm_t *device, uint16_t value);

#ifdef __cplusplus
}
#endif
