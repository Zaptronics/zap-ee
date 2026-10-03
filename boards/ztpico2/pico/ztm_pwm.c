// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/boards/ztpico2/ztm_pwm.h"

/* TLA2518 physical pin 15 is AIN0/GPIO0, wired to PWM POWER_SAFE.
 * Serialize register operations with motherboard ADC conversions. */
static int pwm_power_safe_read(void *context, bool *high)
{
    ztpico2_connector_t *connector = context;
    if (!connector || !high || !connector->board || !connector->board->ready) return -1;
    ztpico2_t *board = connector->board;
    const zap_lock_t *lock = ztmb_zap_lock(&board->adc_lock);
    if (zap_lock_acquire(lock) != 0) return -1;
    uint8_t value = 0;
    int rc = tla2518_read_gpi(&board->adc, &value);
    zap_lock_release(lock);
    if (rc == 0) *high = (value & 1u) != 0;
    return rc;
}

static int pwm_power_safe_prepare(ztpico2_t *board)
{
    const zap_lock_t *lock = ztmb_zap_lock(&board->adc_lock);
    if (zap_lock_acquire(lock) != 0) return -1;
    /* GPIO_CFG (0x07): input first; PIN_CFG (0x05): digital GPIO.
     * Update only GPIO0, preserving all other ADC/GPIO assignments. */
    int rc = tla2518_reg_clear_bits(&board->adc, 0x07u, 0x01u);
    if (rc == 0) rc = tla2518_reg_set_bits(&board->adc, 0x05u, 0x01u);
    zap_lock_release(lock);
    return rc;
}

int ztpico2_ztm_pwm_init(ztpico2_ztm_pwm_t *binding,
                       ztm_pwm_t *module,
                       const ztpico2_ztm_pwm_config_t *config)
{
    if (!binding || !module || !config || !config->board ||
        config->board != ztpico2_active() || !config->board->ready ||
        (config->i2c != i2c0 && config->i2c != i2c1) ||
        config->enable_pin < 0 || config->enable_pin > 15 ||
        config->irq_pin < -1 || config->irq_pin > 15 ||
        config->irq_pin == config->enable_pin) {
        return -1;
    }

    binding->connector = (ztpico2_connector_t){
        .board = config->board,
        .enable_pin = config->enable_pin,
        .input_pin = config->irq_pin,
        .input_active_low = config->irq_active_low,
    };
    if (ztpico2_connector_prepare(&binding->connector) != 0) {
        return -2;
    }

    if (pwm_power_safe_prepare(config->board) != 0) {
        *module = (ztm_pwm_t){0};
        module->last_init_stage = ZTM_PWM_INIT_POWER_SAFE;
        module->last_init_rc = ZTM_PWM_ERR_POWER_SAFE_READ;
        module->power_safe_rc = ZTM_PWM_ERR_POWER_SAFE_READ;
        if (config->event != NULL) {
            const ztm_pwm_event_t event = {
                .type = ZTM_PWM_EVENT_POWER_SAFE_ERROR,
                .detail = (uint32_t)(-ZTM_PWM_ERR_POWER_SAFE_READ),
            };
            config->event(config->event_context, &event);
        }
        return ZTM_PWM_ERR_POWER_SAFE_READ;
    }

    if (!ztmb_zap_i2c_init(&binding->i2c,
                           config->i2c,
                           BUS_I2C_DEFAULT_BAUD_HZ,
                           BUS_I2C_DEFAULT_TIMEOUT_US)) {
        return -3;
    }

    const ztm_pwm_config_t module_config = {
        .i2c = &binding->i2c.bus,
        .time = ztmb_zap_time(),
        .control_context = &binding->connector,
        .power_set = ztpico2_connector_enable_set,
        .power_get = ztpico2_connector_enable_get,
        .power_safe_read = pwm_power_safe_read,
        .irq_read = config->irq_pin >= 0 ? ztpico2_connector_input_read : NULL,
        .event_context = config->event_context,
        .event = config->event,
    };
    return ztm_pwm_init(module, &module_config);
}
