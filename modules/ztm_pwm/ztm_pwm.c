// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/modules/ztm_pwm.h"

#include <string.h>


/* Logical channel -> SX1509 IO pin. */
static const uint8_t g_channel_to_sx_pin[ZTM_PWM_CHANNEL_COUNT] = {
    15u, 14u, 13u, 12u, 11u, 10u, 9u, 7u, 5u, 4u, 2u, 0u
};

/* GPIO1..4 -> SX1509 IO pin. */
static const uint8_t g_gpio_to_sx_pin[ZTM_PWM_GPIO_COUNT] = {
    8u, 6u, 3u, 1u
};

typedef struct {
    ztm_pwm_current_source_t source;
    uint8_t input_channel;
} current_route_t;

/*
 * Intended PAC1954 routing within each four-load group:
 *   logical 1,2,3,4 -> VSENSE2, VSENSE1, VSENSE4, VSENSE3.
 */
static const current_route_t g_native_route[4] = {
    {ZTM_PWM_CURRENT_SOURCE_VSENSE, 2u},
    {ZTM_PWM_CURRENT_SOURCE_VSENSE, 1u},
    {ZTM_PWM_CURRENT_SOURCE_VSENSE, 4u},
    {ZTM_PWM_CURRENT_SOURCE_VSENSE, 3u},
};

/*
 * Salvage routing when PAC1952-2 is fitted in the PAC1954 VQFN footprint:
 *   logical 1 -> VBUS1, logical 2 -> VSENSE1,
 *   logical 3 -> VSENSE2, logical 4 -> VBUS2.
 * VBUS-derived current is valid but about 320x coarser than VSENSE.
 */
static const current_route_t g_pac1952_2_route[4] = {
    {ZTM_PWM_CURRENT_SOURCE_VBUS,   1u},
    {ZTM_PWM_CURRENT_SOURCE_VSENSE, 1u},
    {ZTM_PWM_CURRENT_SOURCE_VSENSE, 2u},
    {ZTM_PWM_CURRENT_SOURCE_VBUS,   2u},
};

typedef struct {
    uint8_t emc_index;
    uint8_t emc_fan;
} fan_route_t;

static const fan_route_t g_fan_route[ZTM_PWM_FAN_COUNT] = {
    {0u, 1u}, {0u, 2u}, {0u, 3u},
    {1u, 1u}, {1u, 2u}, {1u, 3u},
};

static bool valid_channel(uint8_t channel)
{
    return channel >= 1u && channel <= ZTM_PWM_CHANNEL_COUNT;
}

static bool valid_fan(uint8_t fan)
{
    return fan >= 1u && fan <= ZTM_PWM_FAN_COUNT;
}

static bool valid_gpio(uint8_t gpio)
{
    return gpio >= 1u && gpio <= ZTM_PWM_GPIO_COUNT;
}

static bool module_set_enable(ztm_pwm_t *device, bool enabled);
static bool fan_set_output_mode_hw(ztm_pwm_t *device,
                                   uint8_t fan_1based,
                                   ztm_pwm_fan_output_mode_t mode);

const char *ztm_pwm_init_stage_name(ztm_pwm_init_stage_t stage)
{
    switch (stage) {
        case ZTM_PWM_INIT_POWER_OFF: return "power off";
        case ZTM_PWM_INIT_POWER_SAFE: return "POWER_SAFE";
        case ZTM_PWM_INIT_POWER_ON: return "power on";
        case ZTM_PWM_INIT_SX1509: return "SX1509";
        case ZTM_PWM_INIT_LOW_SIDE_OUTPUTS: return "low-side outputs";
        case ZTM_PWM_INIT_USER_GPIOS: return "user GPIOs";
        case ZTM_PWM_INIT_PAC1: return "PAC1";
        case ZTM_PWM_INIT_PAC2: return "PAC2";
        case ZTM_PWM_INIT_PAC3: return "PAC3";
        case ZTM_PWM_INIT_EMC1: return "EMC1";
        case ZTM_PWM_INIT_EMC2: return "EMC2";
        case ZTM_PWM_INIT_FAN_SAFE_STATE: return "fan safe state";
        case ZTM_PWM_INIT_FAN_OUTPUT_CONFIG: return "fan output config";
        case ZTM_PWM_INIT_FAN_INTERRUPTS: return "fan interrupts";
        case ZTM_PWM_INIT_CLEAR_STATUS: return "clear status";
        case ZTM_PWM_INIT_READY: return "ready";
        default: return "not started";
    }
}

const char *ztm_pwm_pac_topology_name(ztm_pwm_pac_topology_t topology)
{
    switch (topology) {
        case ZTM_PWM_PAC_TOPOLOGY_NATIVE_4CH: return "PAC1954 native 4-channel";
        case ZTM_PWM_PAC_TOPOLOGY_PAC1952_2_SALVAGE: return "PAC1952-2 salvage";
        case ZTM_PWM_PAC_TOPOLOGY_UNSUPPORTED: return "unsupported PAC195x";
        default: return "offline";
    }
}

const char *ztm_pwm_current_source_name(ztm_pwm_current_source_t source)
{
    switch (source) {
        case ZTM_PWM_CURRENT_SOURCE_VSENSE: return "VSENSE";
        case ZTM_PWM_CURRENT_SOURCE_VBUS: return "VBUS";
        default: return "none";
    }
}

const char *ztm_pwm_fan_output_mode_name(ztm_pwm_fan_output_mode_t mode)
{
    switch (mode) {
        case ZTM_PWM_FAN_OUTPUT_OPEN_DRAIN: return "open-drain";
        case ZTM_PWM_FAN_OUTPUT_PUSH_PULL: return "push-pull";
        default: return "unknown";
    }
}

static bool fan_set_output_mode_hw(ztm_pwm_t *device,
                                   uint8_t fan_1based,
                                   ztm_pwm_fan_output_mode_t mode)
{
    if (!device || !valid_fan(fan_1based) ||
        (mode != ZTM_PWM_FAN_OUTPUT_OPEN_DRAIN &&
         mode != ZTM_PWM_FAN_OUTPUT_PUSH_PULL)) {
        return false;
    }

    fan_route_t route = g_fan_route[fan_1based - 1u];
    emc2303_pwm_output_type_t type = mode == ZTM_PWM_FAN_OUTPUT_PUSH_PULL
                                         ? EMC2303_PWM_OUTPUT_PUSH_PULL
                                         : EMC2303_PWM_OUTPUT_OPEN_DRAIN;
    return emc2303_fan_set_pwm_output_type(&device->fan_controller[route.emc_index],
                                            route.emc_fan,
                                            type) == EMC2303_OK;
}

static uint8_t pac_channels_from_product_id(uint8_t product_id)
{
    switch (product_id) {
        case 0x78u: case 0x7Cu: return 1u;
        case 0x79u: case 0x7Du: return 2u;
        case 0x7Au: case 0x7Eu: return 3u;
        case 0x7Bu: case 0x7Fu: return 4u;
        default: return 0u;
    }
}

static ztm_pwm_pac_topology_t pac_topology_from_identity(uint8_t product_id,
                                                          uint8_t channels)
{
    if (product_id == 0x7Du && channels == 2u) {
        return ZTM_PWM_PAC_TOPOLOGY_PAC1952_2_SALVAGE;
    }
    if (product_id == 0x7Bu && channels == 4u) {
        return ZTM_PWM_PAC_TOPOLOGY_NATIVE_4CH;
    }
    return ZTM_PWM_PAC_TOPOLOGY_UNSUPPORTED;
}

static int check_power_safe(ztm_pwm_t *device);

static int pwm_init_failed(ztm_pwm_t *device, ztm_pwm_init_stage_t stage, int rc)
{
    if (device != NULL) {
        /* Distinguish reset/power loss during a failed device transaction. */
        if (stage != ZTM_PWM_INIT_POWER_OFF && stage != ZTM_PWM_INIT_POWER_ON &&
            check_power_safe(device) != 0) return device->power_safe_rc;
        device->ready = false;
        device->last_init_stage = stage;
        device->last_init_rc = rc;
        (void)module_set_enable(device, false);
    }
    return rc;
}

static bool module_set_enable(ztm_pwm_t *device, bool enabled)
{
    return device != NULL && device->config.power_set != NULL &&
           device->config.power_set(device->config.control_context, enabled) == 0;
}

/* POWER_SAFE is independent of enable and module IRQ. Never infer it from either. */
static int power_safe_fault(ztm_pwm_t *device, int rc)
{
    const bool first = device->power_safe_rc == 0;
    device->power_safe_rc = rc;
    device->ready = false;
    device->last_init_stage = ZTM_PWM_INIT_POWER_SAFE;
    device->last_init_rc = rc;
    (void)module_set_enable(device, false);
    if (first && device->config.event != NULL) {
        const ztm_pwm_event_t event = {
            .type = ZTM_PWM_EVENT_POWER_SAFE_ERROR,
            .detail = (uint32_t)(-rc),
        };
        device->config.event(device->config.event_context, &event);
    }
    return rc;
}

static int check_power_safe(ztm_pwm_t *device)
{
    if (device == NULL) return ZTM_PWM_ERR_POWER_SAFE_READ;
    if (device->power_safe_rc != 0) return device->power_safe_rc;
    bool high = false;
    if (device->config.power_safe_read == NULL ||
        device->config.power_safe_read(device->config.control_context, &high) != 0)
        return power_safe_fault(device, ZTM_PWM_ERR_POWER_SAFE_READ);
    return high ? 0 : power_safe_fault(device, ZTM_PWM_ERR_POWER_SAFE_LOW);
}

static int wait_power_safe(ztm_pwm_t *device)
{
    device->power_safe_rc = 0;
    if (!zap_time_valid(device->config.time) || device->config.power_safe_read == NULL)
        return power_safe_fault(device, ZTM_PWM_ERR_POWER_SAFE_READ);
    const uint32_t timeout = device->config.power_safe_timeout_ms != 0 ?
        device->config.power_safe_timeout_ms : ZTM_PWM_POWER_SAFE_TIMEOUT_MS;
    const uint64_t start = zap_time_now_us(device->config.time);
    uint32_t waited = 0;
    for (;;) {
        bool high = false;
        if (device->config.power_safe_read(device->config.control_context, &high) != 0)
            return power_safe_fault(device, ZTM_PWM_ERR_POWER_SAFE_READ);
        if (high) return 0;
        if ((device->config.time->now_us != NULL &&
             (zap_time_now_us(device->config.time) - start) / 1000u >= timeout) ||
            waited >= timeout)
            return power_safe_fault(device, ZTM_PWM_ERR_POWER_SAFE_TIMEOUT);
        uint32_t delay = timeout - waited < 5u ? timeout - waited : 5u;
        zap_delay_ms(device->config.time, delay);
        waited += delay;
    }
}

bool ztm_pwm_operational(ztm_pwm_t *device)
{
    return device != NULL && device->ready && check_power_safe(device) == 0;
}

static int init_current_monitor(pac195x_t *monitor,
                                const zap_i2c_bus_t *i2c,
                                const zap_time_t *time,
                                uint8_t address,
                                ztm_pwm_pac_topology_t *topology)
{
    if (monitor == NULL || topology == NULL) {
        return PAC195X_ERR_ARG;
    }
    *topology = ZTM_PWM_PAC_TOPOLOGY_NONE;
    memset(monitor, 0, sizeof(*monitor));

    monitor->set_smbus_settings = true;
    monitor->no_skip = false;
    monitor->byte_count = false;
    monitor->i2c_hispeed = false;

    monitor->set_ctrl = true;
    monitor->sample_mode = PAC195X_SAMPLE_ADAPTIVE_1024;
    /* Zero lets the generic driver enable only channels implemented by the detected part. */
    monitor->enabled_mask = 0u;
    monitor->slow_alert1_cfg = 0u;
    monitor->gpio_alert2_cfg = 0u;

    monitor->set_ranges = true;
    for (size_t i = 0; i < 4u; i++) {
        monitor->vbus_range[i] = PAC195X_VBUS_UNIPOLAR_32V;
        monitor->vsense_range[i] = PAC195X_VSENSE_UNIPOLAR_100mV;
        monitor->shunt_uohm[i] = ZTM_PWM_SHUNT_UOHM;
    }

    monitor->init_refresh_kind = PAC195X_REFRESH_NO_RESET;

    int rc = pac195x_init(monitor, i2c, time, address);
    if (rc != PAC195X_OK) {
        return rc;
    }

    *topology = pac_topology_from_identity(monitor->product_id, monitor->channels);
    return *topology == ZTM_PWM_PAC_TOPOLOGY_UNSUPPORTED ? PAC195X_ERR_ID : PAC195X_OK;
}

static bool channel_start_at_zero(ztm_pwm_t *device, uint8_t channel)
{
    uint8_t pin = g_channel_to_sx_pin[channel - 1u];
    int rc = sx1509_pwm_pin_start(&device->output_mux, pin, 0u);
    if (rc == SX1509_OK) {
        device->channel_duty_pct[channel - 1u] = 0u;
        return true;
    }
    return false;
}

int ztm_pwm_init(ztm_pwm_t *device, const ztm_pwm_config_t *config)
{
    if (device == NULL || config == NULL || !zap_i2c_bus_valid(config->i2c) ||
        !zap_time_valid(config->time) || config->power_set == NULL) {
        return -1;
    }

    memset(device, 0, sizeof(*device));
    device->config = *config;

    device->last_init_stage = ZTM_PWM_INIT_POWER_OFF;
    if (!module_set_enable(device, false)) {
        return pwm_init_failed(device, ZTM_PWM_INIT_POWER_OFF, -3);
    }
    zap_delay_ms(device->config.time, 1u);

    device->last_init_stage = ZTM_PWM_INIT_POWER_ON;
    if (!module_set_enable(device, true)) {
        return pwm_init_failed(device, ZTM_PWM_INIT_POWER_ON, -4);
    }
    device->last_init_stage = ZTM_PWM_INIT_POWER_SAFE;
    int power_rc = wait_power_safe(device);
    if (power_rc != 0) return power_rc;

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->last_init_stage = ZTM_PWM_INIT_SX1509;
    int rc = sx1509_init(&device->output_mux,
                         device->config.i2c,
                         device->config.time,
                         ZTM_PWM_SX1509_ADDR);
    if (rc != SX1509_OK) {
        return pwm_init_failed(device, ZTM_PWM_INIT_SX1509, -10);
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->last_init_stage = ZTM_PWM_INIT_LOW_SIDE_OUTPUTS;
    for (uint8_t channel = 1u; channel <= ZTM_PWM_CHANNEL_COUNT; channel++) {
        if (!channel_start_at_zero(device, channel)) {
            return pwm_init_failed(device, ZTM_PWM_INIT_LOW_SIDE_OUTPUTS, -11);
        }
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->last_init_stage = ZTM_PWM_INIT_USER_GPIOS;
    for (uint8_t gpio = 1u; gpio <= ZTM_PWM_GPIO_COUNT; gpio++) {
        uint8_t pin = g_gpio_to_sx_pin[gpio - 1u];
        if (sx1509_pin_configure(&device->output_mux, pin, SX1509_PIN_INPUT) != SX1509_OK) {
            return pwm_init_failed(device, ZTM_PWM_INIT_USER_GPIOS, -12);
        }
        device->gpio_mode[gpio - 1u] = ZTM_PWM_GPIO_INPUT;
    }

    static const uint8_t pac_addresses[ZTM_PWM_PAC_COUNT] = {
        ZTM_PWM_PAC1_ADDR, ZTM_PWM_PAC2_ADDR, ZTM_PWM_PAC3_ADDR
    };
    static const ztm_pwm_init_stage_t pac_stages[ZTM_PWM_PAC_COUNT] = {
        ZTM_PWM_INIT_PAC1, ZTM_PWM_INIT_PAC2, ZTM_PWM_INIT_PAC3
    };

    for (size_t index = 0; index < ZTM_PWM_PAC_COUNT; index++) {
        if (check_power_safe(device) != 0) return device->power_safe_rc;
        device->last_init_stage = pac_stages[index];
        rc = init_current_monitor(&device->current_monitor[index],
                                  device->config.i2c,
                                  device->config.time,
                                  pac_addresses[index],
                                  &device->current_monitor_topology[index]);
        device->current_monitor_rc[index] = rc;
        device->current_monitor_ready[index] = rc == PAC195X_OK;
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->last_init_stage = ZTM_PWM_INIT_EMC1;
    rc = emc2303_init(&device->fan_controller[0],
                      device->config.i2c,
                      device->config.time,
                      ZTM_PWM_EMC1_ADDR);
    if (rc != EMC2303_OK) {
        return pwm_init_failed(device, ZTM_PWM_INIT_EMC1, -30);
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->last_init_stage = ZTM_PWM_INIT_EMC2;
    rc = emc2303_init(&device->fan_controller[1],
                      device->config.i2c,
                      device->config.time,
                      ZTM_PWM_EMC2_ADDR);
    if (rc != EMC2303_OK) {
        return pwm_init_failed(device, ZTM_PWM_INIT_EMC2, -31);
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->last_init_stage = ZTM_PWM_INIT_FAN_SAFE_STATE;
    for (uint8_t fan = 1u; fan <= ZTM_PWM_FAN_COUNT; fan++) {
        fan_route_t route = g_fan_route[fan - 1u];
        if (emc2303_fan_set_pwm_raw(&device->fan_controller[route.emc_index],
                                    route.emc_fan,
                                    0u) != EMC2303_OK) {
            return pwm_init_failed(device, ZTM_PWM_INIT_FAN_SAFE_STATE, -32);
        }
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->last_init_stage = ZTM_PWM_INIT_FAN_OUTPUT_CONFIG;
    for (uint8_t fan = 1u; fan <= ZTM_PWM_FAN_COUNT; fan++) {
        ztm_pwm_fan_output_mode_t mode =
            (ZTM_PWM_FAN_DEFAULT_PUSH_PULL_MASK & (1u << (fan - 1u))) != 0u
                ? ZTM_PWM_FAN_OUTPUT_PUSH_PULL
                : ZTM_PWM_FAN_OUTPUT_OPEN_DRAIN;
        if (!fan_set_output_mode_hw(device, fan, mode)) {
            return pwm_init_failed(device, ZTM_PWM_INIT_FAN_OUTPUT_CONFIG, -33);
        }
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->last_init_stage = ZTM_PWM_INIT_FAN_INTERRUPTS;
    if (emc2303_reg_write8(&device->fan_controller[0], EMC2303_REG_FAN_INT_ENABLE, 0x07u) != EMC2303_OK ||
        emc2303_reg_write8(&device->fan_controller[1], EMC2303_REG_FAN_INT_ENABLE, 0x07u) != EMC2303_OK) {
        return pwm_init_failed(device, ZTM_PWM_INIT_FAN_INTERRUPTS, -34);
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->last_init_stage = ZTM_PWM_INIT_CLEAR_STATUS;
    uint8_t unused = 0u;
    (void)emc2303_read_status(&device->fan_controller[0], &unused, NULL, NULL, NULL);
    (void)emc2303_read_status(&device->fan_controller[1], &unused, NULL, NULL, NULL);
    for (size_t i = 0; i < ZTM_PWM_PAC_COUNT; i++) {
        if (device->current_monitor_ready[i]) {
            uint32_t alert = 0u;
            (void)pac195x_read_alert_status(&device->current_monitor[i], &alert);
        }
    }
    uint16_t sx_source = 0u;
    if (sx1509_interrupt_get_source(&device->output_mux, &sx_source) == SX1509_OK && sx_source != 0u) {
        (void)sx1509_interrupt_clear_source(&device->output_mux, sx_source);
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    device->ready = true;
    device->last_init_stage = ZTM_PWM_INIT_READY;
    device->last_init_rc = 0;
    return 0;
}

bool ztm_pwm_ready(const ztm_pwm_t *device)
{
    return device != NULL && device->ready;
}

bool ztm_pwm_read_enable(const ztm_pwm_t *device, bool *enabled)
{
    if (device == NULL || enabled == NULL || device->config.power_get == NULL) {
        return false;
    }
    return device->config.power_get(device->config.control_context, enabled) == 0;
}

bool ztm_pwm_read_irq_asserted(const ztm_pwm_t *device, bool *asserted)
{
    if (device == NULL || asserted == NULL || device->config.irq_read == NULL) {
        return false;
    }
    return device->config.irq_read(device->config.control_context, asserted) == 0;
}

int ztm_pwm_probe_devices(ztm_pwm_t *device,
                          ztm_pwm_probe_result_t *result,
                          bool ensure_power_on)
{
    if (result == NULL) return -1;
    memset(result, 0, sizeof(*result));
    result->power_safe_rc = ZTM_PWM_ERR_POWER_SAFE_READ;
    if (device == NULL || result == NULL || !zap_i2c_bus_valid(device->config.i2c)) {
        return -1;
    }

    bool original_enable = false;
    bool original_enable_valid = ztm_pwm_read_enable(device, &original_enable);
    result->power_was_enabled = original_enable_valid && original_enable;
    result->power_control_ok = device->config.power_set != NULL;

    if (ensure_power_on && (!original_enable_valid || !original_enable)) {
        if (!module_set_enable(device, true)) {
            result->power_control_ok = false;
            return -3;
        }
        result->power_temporarily_enabled = true;
    }

    result->power_safe_rc = device->ready ? check_power_safe(device) : wait_power_safe(device);
    if (result->power_safe_rc != 0) goto power_cleanup;

    result->enable_read_ok = ztm_pwm_read_enable(device, &result->enable_high);
    result->irq_read_ok = ztm_pwm_read_irq_asserted(device, &result->irq_asserted);

    sx1509_t sx = {0};
    if (zap_i2c_device_init(&sx.i2c, device->config.i2c, ZTM_PWM_SX1509_ADDR) == ZAP_I2C_OK) {
        sx.time = device->config.time;
        if (sx1509_reg_read8(&sx, SX1509_REG_DIR_A, &result->sx_dir_a) == SX1509_OK) {
            result->sx_present = true;
            result->devices_found++;
        }
    }

    static const uint8_t pac_addresses[ZTM_PWM_PAC_COUNT] = {
        ZTM_PWM_PAC1_ADDR, ZTM_PWM_PAC2_ADDR, ZTM_PWM_PAC3_ADDR
    };
    for (size_t index = 0; index < ZTM_PWM_PAC_COUNT; index++) {
        if ((result->power_safe_rc = check_power_safe(device)) != 0) goto power_cleanup;
        pac195x_t pac = {0};
        if (zap_i2c_device_init(&pac.i2c, device->config.i2c, pac_addresses[index]) != ZAP_I2C_OK) {
            continue;
        }
        pac.time = device->config.time;
        int rc = pac195x_reg_read_u8(&pac, PAC195X_REG_MANUFACTURER_ID,
                                     &result->pac_manufacturer_id[index]);
        if (rc == PAC195X_OK) {
            rc = pac195x_reg_read_u8(&pac, PAC195X_REG_PRODUCT_ID,
                                     &result->pac_product_id[index]);
        }
        if (rc == PAC195X_OK) {
            rc = pac195x_reg_read_u8(&pac, PAC195X_REG_REVISION_ID,
                                     &result->pac_revision_id[index]);
        }
        if (rc == PAC195X_OK) {
            result->pac_present[index] = true;
            result->pac_channels[index] = pac_channels_from_product_id(result->pac_product_id[index]);
            result->devices_found++;
        }
    }

    static const uint8_t emc_addresses[ZTM_PWM_EMC_COUNT] = {
        ZTM_PWM_EMC1_ADDR, ZTM_PWM_EMC2_ADDR
    };
    for (size_t index = 0; index < ZTM_PWM_EMC_COUNT; index++) {
        if ((result->power_safe_rc = check_power_safe(device)) != 0) goto power_cleanup;
        emc2303_t emc = {0};
        if (zap_i2c_device_init(&emc.i2c, device->config.i2c, emc_addresses[index]) != ZAP_I2C_OK) {
            continue;
        }
        emc.time = device->config.time;
        if (emc2303_read_ids(&emc,
                             &result->emc_product_id[index],
                             &result->emc_manufacturer_id[index],
                             &result->emc_revision_id[index]) == EMC2303_OK) {
            result->emc_present[index] = true;
            result->devices_found++;
        }
    }

    result->power_safe_rc = check_power_safe(device);
power_cleanup:
    if (result->power_temporarily_enabled) {
        result->power_restored_off = module_set_enable(device, false);
        if (!result->power_restored_off) {
            result->power_control_ok = false;
            return -4;
        }
    }

    if (result->power_safe_rc != 0) return result->power_safe_rc;
    return result->devices_found == 6u ? 0 : -(int)(6u - result->devices_found);
}

int ztm_pwm_i2c_scan(ztm_pwm_t *device,
                     ztm_pwm_i2c_scan_result_t *result,
                     bool ensure_power_on)
{
    if (result == NULL) return -1;
    memset(result, 0, sizeof(*result));
    result->power_safe_rc = ZTM_PWM_ERR_POWER_SAFE_READ;
    if (device == NULL || result == NULL || !zap_i2c_bus_valid(device->config.i2c)) {
        return -1;
    }
    if (device->config.i2c->ops->probe == NULL) {
        return ZAP_I2C_E_UNSUPPORTED;
    }

    bool original_enable = false;
    bool original_enable_valid = ztm_pwm_read_enable(device, &original_enable);
    result->power_was_enabled = original_enable_valid && original_enable;
    result->power_control_ok = device->config.power_set != NULL;

    if (ensure_power_on && (!original_enable_valid || !original_enable)) {
        if (!module_set_enable(device, true)) {
            result->power_control_ok = false;
            return -3;
        }
        result->power_temporarily_enabled = true;
    }

    result->power_safe_rc = device->ready ? check_power_safe(device) : wait_power_safe(device);
    if (result->power_safe_rc != 0) goto power_cleanup;

    for (uint8_t address = 0x08u; address <= 0x77u; address++) {
        if ((result->power_safe_rc = check_power_safe(device)) != 0) goto power_cleanup;
        zap_i2c_device_t probe = {0};
        if (zap_i2c_device_init(&probe, device->config.i2c, address) == ZAP_I2C_OK &&
            zap_i2c_probe(&probe) == ZAP_I2C_OK) {
            result->address_present[address] = true;
            result->devices_found++;
        }
    }

    result->power_safe_rc = check_power_safe(device);
power_cleanup:
    if (result->power_temporarily_enabled) {
        result->power_restored_off = module_set_enable(device, false);
        if (!result->power_restored_off) {
            result->power_control_ok = false;
            return -4;
        }
    }

    if (result->power_safe_rc != 0) return result->power_safe_rc;
    return (int)result->devices_found;
}

bool ztm_pwm_power_off(ztm_pwm_t *device)
{
    if (device == NULL) {
        return false;
    }
    bool stopped = !device->ready || ztm_pwm_stop_all(device);
    bool disabled = module_set_enable(device, false);
    device->ready = false;
    if (disabled) device->power_safe_rc = 0;
    return stopped && disabled;
}

bool ztm_pwm_reinitialize(ztm_pwm_t *device)
{
    if (device == NULL) {
        return false;
    }
    ztm_pwm_config_t config = device->config;
    if (!zap_i2c_bus_valid(config.i2c) || config.power_set == NULL) {
        return false;
    }
    (void)ztm_pwm_power_off(device);
    return ztm_pwm_init(device, &config) == 0;
}

uint8_t ztm_pwm_channel_get_sx_pin(uint8_t channel_1based)
{
    return valid_channel(channel_1based) ? g_channel_to_sx_pin[channel_1based - 1u] : UINT8_MAX;
}

uint8_t ztm_pwm_gpio_get_sx_pin(uint8_t gpio_1based)
{
    return valid_gpio(gpio_1based) ? g_gpio_to_sx_pin[gpio_1based - 1u] : UINT8_MAX;
}

static bool resolve_current_route(const ztm_pwm_t *device,
                                  uint8_t channel_1based,
                                  size_t *pac_index,
                                  current_route_t *route)
{
    if (!device || !valid_channel(channel_1based) || !pac_index || !route) {
        return false;
    }

    size_t pac = (size_t)((channel_1based - 1u) / 4u);
    size_t local = (size_t)((channel_1based - 1u) % 4u);
    const current_route_t *table = NULL;
    switch (device->current_monitor_topology[pac]) {
        case ZTM_PWM_PAC_TOPOLOGY_NATIVE_4CH:
            table = g_native_route;
            break;
        case ZTM_PWM_PAC_TOPOLOGY_PAC1952_2_SALVAGE:
            table = g_pac1952_2_route;
            break;
        default:
            return false;
    }

    *pac_index = pac;
    *route = table[local];
    return true;
}

bool ztm_pwm_channel_get_current_route(const ztm_pwm_t *device,
                                       uint8_t channel_1based,
                                       uint8_t *pac_1based,
                                       ztm_pwm_current_source_t *source,
                                       uint8_t *input_1based)
{
    if (!pac_1based || !source || !input_1based) {
        return false;
    }

    size_t pac = 0u;
    current_route_t route;
    if (!resolve_current_route(device, channel_1based, &pac, &route)) {
        return false;
    }

    *pac_1based = (uint8_t)(pac + 1u);
    *source = route.source;
    *input_1based = route.input_channel;
    return true;
}

bool ztm_pwm_channel_set(ztm_pwm_t *device, uint8_t channel_1based, uint8_t duty_pct)
{
    if (!ztm_pwm_operational(device) || !valid_channel(channel_1based) || duty_pct > 100u) {
        return false;
    }

    uint8_t pin = g_channel_to_sx_pin[channel_1based - 1u];
    if (sx1509_pwm_pin_set_duty(&device->output_mux, pin, duty_pct) != SX1509_OK) {
        return false;
    }

    device->channel_duty_pct[channel_1based - 1u] = duty_pct;
    return true;
}

bool ztm_pwm_channel_set_all(ztm_pwm_t *device, uint8_t duty_pct)
{
    if (!ztm_pwm_operational(device) || duty_pct > 100u) {
        return false;
    }

    bool ok = true;
    for (uint8_t channel = 1u; channel <= ZTM_PWM_CHANNEL_COUNT; channel++) {
        ok = ztm_pwm_channel_set(device, channel, duty_pct) && ok;
    }
    return ok;
}

bool ztm_pwm_channel_stop(ztm_pwm_t *device, uint8_t channel_1based)
{
    /* Zero sink duty releases the MCP1415 input; its pull-up holds the MOSFET off. */
    return ztm_pwm_channel_set(device, channel_1based, 0u);
}

bool ztm_pwm_stop_all(ztm_pwm_t *device)
{
    /* An unavailable module cannot confirm zero-duty writes. */
    if (!ztm_pwm_operational(device)) return false;
    bool outputs_ok = ztm_pwm_channel_set_all(device, 0u);
    bool fans_ok = ztm_pwm_fan_stop_all(device);
    return outputs_ok && fans_ok;
}

uint8_t ztm_pwm_channel_get_duty(const ztm_pwm_t *device, uint8_t channel_1based)
{
    return valid_channel(channel_1based) ? device->channel_duty_pct[channel_1based - 1u] : 0u;
}

bool ztm_pwm_read_channel(ztm_pwm_t *device,
                          uint8_t channel_1based,
                          bool averaged,
                          ztm_pwm_current_reading_t *reading)
{
    if (!ztm_pwm_operational(device) || !valid_channel(channel_1based) || !reading) {
        return false;
    }

    size_t pac = 0u;
    current_route_t route;
    if (!resolve_current_route(device, channel_1based, &pac, &route) ||
        !device->current_monitor_ready[pac]) {
        return false;
    }

    pac195x_t *monitor = &device->current_monitor[pac];
    if (pac195x_refresh(monitor, PAC195X_REFRESH_NO_RESET, true) != PAC195X_OK) {
        return false;
    }

    pac195x_shunt_reading_t shunt = {0};
    int rc;
    if (route.source == ZTM_PWM_CURRENT_SOURCE_VBUS) {
        rc = pac195x_read_vbus_shunt(monitor,
                                     route.input_channel,
                                     averaged,
                                     ZTM_PWM_SHUNT_UOHM,
                                     &shunt);
    } else if (route.source == ZTM_PWM_CURRENT_SOURCE_VSENSE) {
        rc = pac195x_read_vsense_shunt(monitor,
                                       route.input_channel,
                                       averaged,
                                       ZTM_PWM_SHUNT_UOHM,
                                       &shunt);
    } else {
        return false;
    }
    if (rc != PAC195X_OK) {
        return false;
    }

    reading->pac_1based = (uint8_t)(pac + 1u);
    reading->input_1based = route.input_channel;
    reading->source = route.source;
    reading->raw = shunt.raw;
    reading->shunt_uV = shunt.shunt_uV;
    reading->current_uA = shunt.current_uA;
    reading->coarse_resolution = route.source == ZTM_PWM_CURRENT_SOURCE_VBUS;
    return true;
}

bool ztm_pwm_read_current_uA(ztm_pwm_t *device, uint8_t channel_1based, bool averaged, int32_t *current_uA)
{
    if (!current_uA) {
        return false;
    }
    ztm_pwm_current_reading_t reading;
    if (!ztm_pwm_read_channel(device, channel_1based, averaged, &reading)) {
        return false;
    }
    *current_uA = reading.current_uA;
    return true;
}

bool ztm_pwm_read_all_currents_uA(ztm_pwm_t *device, bool averaged, int32_t current_uA[ZTM_PWM_CHANNEL_COUNT])
{
    if (!ztm_pwm_operational(device) || !current_uA) {
        return false;
    }

    bool all_ok = true;
    for (uint8_t channel = 1u; channel <= ZTM_PWM_CHANNEL_COUNT; channel++) {
        if (!ztm_pwm_read_current_uA(device, channel, averaged, &current_uA[channel - 1u])) {
            current_uA[channel - 1u] = 0;
            all_ok = false;
        }
    }
    return all_ok;
}

bool ztm_pwm_fan_get_route(uint8_t fan_1based,
                           uint8_t *emc_1based,
                           uint8_t *local_fan_1based)
{
    if (!valid_fan(fan_1based) || !emc_1based || !local_fan_1based) {
        return false;
    }
    fan_route_t route = g_fan_route[fan_1based - 1u];
    *emc_1based = (uint8_t)(route.emc_index + 1u);
    *local_fan_1based = route.emc_fan;
    return true;
}

bool ztm_pwm_fan_set_raw(ztm_pwm_t *device, uint8_t fan_1based, uint8_t raw)
{
    if (!ztm_pwm_operational(device) || !valid_fan(fan_1based)) {
        return false;
    }

    fan_route_t route = g_fan_route[fan_1based - 1u];
    if (emc2303_fan_enable_closed_loop(&device->fan_controller[route.emc_index],
                                        route.emc_fan,
                                        false) != EMC2303_OK ||
        emc2303_fan_set_pwm_raw(&device->fan_controller[route.emc_index],
                                route.emc_fan,
                                raw) != EMC2303_OK) {
        return false;
    }

    device->fan_duty_pct[fan_1based - 1u] =
        (uint8_t)(((uint32_t)raw * 100u + 127u) / 255u);
    device->fan_closed_loop[fan_1based - 1u] = false;
    device->fan_target_rpm[fan_1based - 1u] = 0u;
    return true;
}

bool ztm_pwm_fan_set_open_loop(ztm_pwm_t *device, uint8_t fan_1based, uint8_t duty_pct)
{
    return ztm_pwm_fan_set(device, fan_1based, duty_pct);
}

bool ztm_pwm_fan_set_target_rpm(ztm_pwm_t *device, uint8_t fan_1based, uint32_t rpm)
{
    if (!ztm_pwm_operational(device) || !valid_fan(fan_1based) || rpm == 0u) {
        return false;
    }
    fan_route_t route = g_fan_route[fan_1based - 1u];
    if (emc2303_fan_set_target_rpm(&device->fan_controller[route.emc_index],
                                   route.emc_fan,
                                   rpm) != EMC2303_OK) {
        return false;
    }
    device->fan_closed_loop[fan_1based - 1u] = true;
    device->fan_target_rpm[fan_1based - 1u] = rpm;
    return true;
}

bool ztm_pwm_fan_set_min_drive(ztm_pwm_t *device, uint8_t fan_1based, uint8_t duty_pct)
{
    if (!ztm_pwm_operational(device) || !valid_fan(fan_1based) || duty_pct > 100u) {
        return false;
    }
    fan_route_t route = g_fan_route[fan_1based - 1u];
    uint8_t raw = (uint8_t)(((uint32_t)duty_pct * 255u + 50u) / 100u);
    return emc2303_fan_set_min_drive(&device->fan_controller[route.emc_index],
                                     route.emc_fan,
                                     raw) == EMC2303_OK;
}

bool ztm_pwm_fan_set(ztm_pwm_t *device, uint8_t fan_1based, uint8_t duty_pct)
{
    if (!ztm_pwm_operational(device) || !valid_fan(fan_1based) || duty_pct > 100u) {
        return false;
    }

    fan_route_t route = g_fan_route[fan_1based - 1u];
    if (emc2303_fan_enable_closed_loop(&device->fan_controller[route.emc_index],
                                        route.emc_fan,
                                        false) != EMC2303_OK ||
        emc2303_fan_set_pwm_percent_x100(&device->fan_controller[route.emc_index],
                                         route.emc_fan,
                                         (uint16_t)duty_pct * 100u) != EMC2303_OK) {
        return false;
    }

    device->fan_duty_pct[fan_1based - 1u] = duty_pct;
    device->fan_closed_loop[fan_1based - 1u] = false;
    device->fan_target_rpm[fan_1based - 1u] = 0u;
    return true;
}

bool ztm_pwm_fan_set_all(ztm_pwm_t *device, uint8_t duty_pct)
{
    if (!ztm_pwm_operational(device) || duty_pct > 100u) {
        return false;
    }

    bool ok = true;
    for (uint8_t fan = 1u; fan <= ZTM_PWM_FAN_COUNT; fan++) {
        ok = ztm_pwm_fan_set(device, fan, duty_pct) && ok;
    }
    return ok;
}

bool ztm_pwm_fan_set_output_mode(ztm_pwm_t *device, uint8_t fan_1based, ztm_pwm_fan_output_mode_t mode)
{
    return ztm_pwm_operational(device) && fan_set_output_mode_hw(device, fan_1based, mode);
}

bool ztm_pwm_fan_set_output_mode_all(ztm_pwm_t *device, ztm_pwm_fan_output_mode_t mode)
{
    if (!ztm_pwm_operational(device) ||
        (mode != ZTM_PWM_FAN_OUTPUT_OPEN_DRAIN &&
         mode != ZTM_PWM_FAN_OUTPUT_PUSH_PULL)) {
        return false;
    }

    bool ok = true;
    for (uint8_t fan = 1u; fan <= ZTM_PWM_FAN_COUNT; fan++) {
        ok = fan_set_output_mode_hw(device, fan, mode) && ok;
    }
    return ok;
}

bool ztm_pwm_fan_get_output_mode(ztm_pwm_t *device, uint8_t fan_1based, ztm_pwm_fan_output_mode_t *mode)
{
    if (!ztm_pwm_operational(device) || !valid_fan(fan_1based) || !mode) {
        return false;
    }

    fan_route_t route = g_fan_route[fan_1based - 1u];
    emc2303_pwm_output_type_t type = EMC2303_PWM_OUTPUT_OPEN_DRAIN;
    if (emc2303_fan_get_pwm_output_type(&device->fan_controller[route.emc_index],
                                         route.emc_fan,
                                         &type) != EMC2303_OK) {
        return false;
    }

    *mode = type == EMC2303_PWM_OUTPUT_PUSH_PULL
                ? ZTM_PWM_FAN_OUTPUT_PUSH_PULL
                : ZTM_PWM_FAN_OUTPUT_OPEN_DRAIN;
    return true;
}

bool ztm_pwm_fan_stop(ztm_pwm_t *device, uint8_t fan_1based)
{
    return ztm_pwm_fan_set(device, fan_1based, 0u);
}

bool ztm_pwm_fan_stop_all(ztm_pwm_t *device)
{
    return ztm_pwm_fan_set_all(device, 0u);
}

bool ztm_pwm_fan_get_rpm(ztm_pwm_t *device, uint8_t fan_1based, uint32_t *rpm)
{
    uint16_t tach_count = 0u;
    return ztm_pwm_fan_get_tach_status(device, fan_1based, &tach_count, rpm) !=
           ZTM_PWM_FAN_TACH_READ_ERROR;
}

bool ztm_pwm_fan_get_tach(ztm_pwm_t *device, uint8_t fan_1based, uint16_t *tach_count, uint32_t *rpm)
{
    return ztm_pwm_fan_get_tach_status(device, fan_1based, tach_count, rpm) !=
           ZTM_PWM_FAN_TACH_READ_ERROR;
}

ztm_pwm_fan_tach_status_t ztm_pwm_fan_get_tach_status(ztm_pwm_t *device,
                                                       uint8_t fan_1based,
                                                       uint16_t *tach_count,
                                                       uint32_t *rpm)
{
    if (!ztm_pwm_operational(device) || !valid_fan(fan_1based) || !tach_count || !rpm) {
        return ZTM_PWM_FAN_TACH_READ_ERROR;
    }

    fan_route_t route = g_fan_route[fan_1based - 1u];
    emc2303_t *controller = &device->fan_controller[route.emc_index];
    if (emc2303_fan_get_tach_counts(controller, route.emc_fan, tach_count) != EMC2303_OK) {
        return ZTM_PWM_FAN_TACH_READ_ERROR;
    }

    if (*tach_count == EMC2303_TACH_COUNT_STALL) {
        *rpm = 0u;
        return ZTM_PWM_FAN_TACH_STALL;
    }

    if (emc2303_fan_tach_counts_to_rpm(controller,
                                         route.emc_fan,
                                         *tach_count,
                                         rpm) != EMC2303_OK) {
        return ZTM_PWM_FAN_TACH_READ_ERROR;
    }

    return ZTM_PWM_FAN_TACH_OK;
}

uint8_t ztm_pwm_fan_get_duty(const ztm_pwm_t *device, uint8_t fan_1based)
{
    return valid_fan(fan_1based) ? device->fan_duty_pct[fan_1based - 1u] : 0u;
}

bool ztm_pwm_gpio_configure_input(ztm_pwm_t *device, uint8_t gpio_1based, ztm_pwm_gpio_pull_t pull)
{
    if (!ztm_pwm_operational(device) || !valid_gpio(gpio_1based)) {
        return false;
    }

    uint32_t flags = SX1509_PIN_INPUT;
    if (pull == ZTM_PWM_GPIO_PULL_UP) {
        flags |= SX1509_PIN_PULLUP;
    } else if (pull == ZTM_PWM_GPIO_PULL_DOWN) {
        flags |= SX1509_PIN_PULLDOWN;
    } else if (pull != ZTM_PWM_GPIO_PULL_NONE) {
        return false;
    }

    uint8_t pin = g_gpio_to_sx_pin[gpio_1based - 1u];
    if (sx1509_pin_configure(&device->output_mux, pin, flags) != SX1509_OK) {
        return false;
    }
    device->gpio_mode[gpio_1based - 1u] = ZTM_PWM_GPIO_INPUT;
    device->gpio_pull[gpio_1based - 1u] = pull;
    return true;
}

bool ztm_pwm_gpio_configure(ztm_pwm_t *device, uint8_t gpio_1based, ztm_pwm_gpio_mode_t mode)
{
    if (!ztm_pwm_operational(device) || !valid_gpio(gpio_1based)) {
        return false;
    }

    if (mode == ZTM_PWM_GPIO_INPUT) {
        return ztm_pwm_gpio_configure_input(device, gpio_1based, ZTM_PWM_GPIO_PULL_NONE);
    }

    uint8_t pin = g_gpio_to_sx_pin[gpio_1based - 1u];
    (void)sx1509_interrupt_configure(&device->output_mux, pin, false, SX1509_SENSE_NONE);
    int rc = sx1509_digital_write(&device->output_mux, pin, false);
    if (rc == SX1509_OK) {
        rc = sx1509_pin_configure(&device->output_mux, pin, SX1509_PIN_OUTPUT);
    }
    if (rc != SX1509_OK) {
        return false;
    }

    device->gpio_mode[gpio_1based - 1u] = ZTM_PWM_GPIO_OUTPUT;
    device->gpio_pull[gpio_1based - 1u] = ZTM_PWM_GPIO_PULL_NONE;
    device->gpio_irq_sense[gpio_1based - 1u] = SX1509_SENSE_NONE;
    device->gpio_level[gpio_1based - 1u] = false;
    return true;
}

bool ztm_pwm_gpio_configure_irq(ztm_pwm_t *device, uint8_t gpio_1based, sx1509_sense_t sense)
{
    if (!ztm_pwm_operational(device) || !valid_gpio(gpio_1based) ||
        device->gpio_mode[gpio_1based - 1u] != ZTM_PWM_GPIO_INPUT ||
        sense > SX1509_SENSE_BOTH) {
        return false;
    }
    uint8_t pin = g_gpio_to_sx_pin[gpio_1based - 1u];
    if (sx1509_interrupt_configure(&device->output_mux,
                                    pin,
                                    sense != SX1509_SENSE_NONE,
                                    sense) != SX1509_OK) {
        return false;
    }
    device->gpio_irq_sense[gpio_1based - 1u] = sense;
    return true;
}

bool ztm_pwm_gpio_toggle(ztm_pwm_t *device, uint8_t gpio_1based)
{
    if (!ztm_pwm_operational(device) || !valid_gpio(gpio_1based) ||
        device->gpio_mode[gpio_1based - 1u] != ZTM_PWM_GPIO_OUTPUT) {
        return false;
    }
    uint8_t pin = g_gpio_to_sx_pin[gpio_1based - 1u];
    if (sx1509_digital_toggle(&device->output_mux, pin) != SX1509_OK) {
        return false;
    }
    bool level = false;
    if (sx1509_digital_read(&device->output_mux, pin, &level) != SX1509_OK) {
        return false;
    }
    device->gpio_level[gpio_1based - 1u] = level;
    return true;
}

bool ztm_pwm_gpio_write(ztm_pwm_t *device, uint8_t gpio_1based, bool level)
{
    if (!ztm_pwm_operational(device) || !valid_gpio(gpio_1based) ||
        device->gpio_mode[gpio_1based - 1u] != ZTM_PWM_GPIO_OUTPUT) {
        return false;
    }

    uint8_t pin = g_gpio_to_sx_pin[gpio_1based - 1u];
    if (sx1509_digital_write(&device->output_mux, pin, level) != SX1509_OK) {
        return false;
    }
    device->gpio_level[gpio_1based - 1u] = level;
    return true;
}

bool ztm_pwm_gpio_read(ztm_pwm_t *device, uint8_t gpio_1based, bool *level)
{
    if (!ztm_pwm_operational(device) || !valid_gpio(gpio_1based) || !level) {
        return false;
    }

    uint8_t pin = g_gpio_to_sx_pin[gpio_1based - 1u];
    if (sx1509_digital_read(&device->output_mux, pin, level) != SX1509_OK) {
        return false;
    }
    device->gpio_level[gpio_1based - 1u] = *level;
    return true;
}

static void emit_event(ztm_pwm_t *device,
                       ztm_pwm_event_type_t type,
                       uint8_t device_1based,
                       uint8_t channel_1based,
                       uint32_t detail)
{
    if (device == NULL || device->config.event == NULL) {
        return;
    }
    const ztm_pwm_event_t event = {
        .type = type,
        .device_1based = device_1based,
        .channel_1based = channel_1based,
        .detail = detail,
    };
    device->config.event(device->config.event_context, &event);
}

static void emit_emc_status(ztm_pwm_t *device,
                            uint8_t controller_index,
                            uint8_t fan_status,
                            uint8_t stall,
                            uint8_t spin,
                            uint8_t drive_fail)
{
    uint8_t fan_base = (uint8_t)(controller_index * 3u);
    if ((fan_status & EMC2303_FAN_STATUS_WATCH) != 0u) {
        emit_event(device,
                   ZTM_PWM_EVENT_FAN_WATCHDOG,
                   (uint8_t)(controller_index + 1u),
                   0u,
                   fan_status);
    }

    /* A missing or deliberately unmonitored TACH input can hold stall active.
     * Stall is intentionally observable through explicit tach/status reads but
     * is not emitted as a periodic event to avoid a permanent event storm. */
    (void)stall;

    for (uint8_t local = 1u; local <= 3u; local++) {
        uint8_t bit = EMC2303_FAN_BIT(local);
        uint8_t fan = (uint8_t)(fan_base + local);
        if ((spin & bit) != 0u) {
            emit_event(device,
                       ZTM_PWM_EVENT_FAN_SPINUP_FAILURE,
                       (uint8_t)(controller_index + 1u),
                       fan,
                       spin);
        }
        if ((drive_fail & bit) != 0u) {
            emit_event(device,
                       ZTM_PWM_EVENT_FAN_DRIVE_FAILURE,
                       (uint8_t)(controller_index + 1u),
                       fan,
                       drive_fail);
        }
    }
}

int ztm_pwm_scan_irq_sources(ztm_pwm_t *device, bool force_scan)
{
    if (device == NULL) return -1;
    if (!ztm_pwm_operational(device))
        return device->power_safe_rc != 0 ? device->power_safe_rc : -1;

    bool irq_asserted = false;
    if (!ztm_pwm_read_irq_asserted(device, &irq_asserted)) {
        device->irq_scan_errors++;
        return -2;
    }
    if (!irq_asserted) {
        device->irq_unresolved_reported = false;
        if (!force_scan) {
            return 0;
        }
    }

    device->irq_count++;
    bool source_found = false;
    int errors = 0;

    uint16_t sx_source = 0u;
    int sx_rc = sx1509_interrupt_get_source(&device->output_mux, &sx_source);
    if (sx_rc != SX1509_OK) {
        errors++;
    } else if (sx_source != 0u) {
        source_found = true;
        emit_event(device, ZTM_PWM_EVENT_SX1509_INTERRUPT, 1u, 0u, sx_source);
        if (sx1509_interrupt_clear_source(&device->output_mux, sx_source) != SX1509_OK) {
            errors++;
        }
    }

    for (size_t pac = 0; pac < ZTM_PWM_PAC_COUNT; pac++) {
        if (!device->current_monitor_ready[pac]) {
            continue;
        }
        if (check_power_safe(device) != 0) return device->power_safe_rc;
        uint32_t status = 0u;
        int rc = pac195x_read_alert_status(&device->current_monitor[pac], &status);
        if (rc != PAC195X_OK) {
            errors++;
        } else if (status != 0u) {
            source_found = true;
            emit_event(device,
                       ZTM_PWM_EVENT_PAC_ALERT,
                       (uint8_t)(pac + 1u),
                       0u,
                       status & 0x00ffffffu);
        }
    }

    for (size_t emc = 0; emc < ZTM_PWM_EMC_COUNT; emc++) {
        if (check_power_safe(device) != 0) return device->power_safe_rc;
        uint8_t fan_status = 0u;
        uint8_t stall = 0u;
        uint8_t spin = 0u;
        uint8_t drive_fail = 0u;
        int rc = emc2303_read_status(&device->fan_controller[emc],
                                     &fan_status,
                                     &stall,
                                     &spin,
                                     &drive_fail);
        if (rc != EMC2303_OK) {
            errors++;
        } else if (fan_status != 0u || stall != 0u || spin != 0u || drive_fail != 0u) {
            source_found = true;
            emit_emc_status(device, (uint8_t)emc, fan_status, stall, spin, drive_fail);
        }
    }

    if (irq_asserted && !source_found && errors == 0 &&
        !device->irq_unresolved_reported) {
        emit_event(device, ZTM_PWM_EVENT_IRQ_UNRESOLVED, 0u, 0u, 0u);
        device->irq_unresolved_reported = true;
    }

    if (check_power_safe(device) != 0) return device->power_safe_rc;
    if (errors != 0) {
        device->irq_scan_errors += (uint32_t)errors;
        return -errors;
    }
    return 0;
}

int ztm_pwm_poll(ztm_pwm_t *device)
{
    /* An intentional console power-off is not a periodic poll failure. */
    if (device == NULL) return -1;
    if (device->power_safe_rc != 0) return device->power_safe_rc;
    return device->ready ? ztm_pwm_scan_irq_sources(device, false) : 0;
}

int ztm_pwm_deinit(ztm_pwm_t *device)
{
    return ztm_pwm_power_off(device) ? 0 : -1;
}

bool ztm_pwm_test(ztm_pwm_t *device, uint16_t value)
{
    uint8_t duty = value > 100u ? 100u : (uint8_t)value;
    return ztm_pwm_channel_set_all(device, duty);
}
