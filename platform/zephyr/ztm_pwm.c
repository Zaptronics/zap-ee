// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/zephyr/ztm_pwm.h"

static int zephyr_power_set(void *context, bool enabled)
{
    zap_zephyr_ztm_pwm_adapter_t *adapter =
        (zap_zephyr_ztm_pwm_adapter_t *)context;
    if (adapter == NULL || !adapter->initialized) {
        return -1;
    }
    if (gpio_pin_set_dt(&adapter->power, enabled ? 1 : 0) != 0) {
        return -1;
    }
    adapter->power_enabled = enabled;
    return 0;
}

static int zephyr_power_get(void *context, bool *enabled)
{
    zap_zephyr_ztm_pwm_adapter_t *adapter =
        (zap_zephyr_ztm_pwm_adapter_t *)context;
    if (adapter == NULL || !adapter->initialized || enabled == NULL) {
        return -1;
    }
    *enabled = adapter->power_enabled;
    return 0;
}

static int zephyr_irq_read(void *context, bool *asserted)
{
    zap_zephyr_ztm_pwm_adapter_t *adapter =
        (zap_zephyr_ztm_pwm_adapter_t *)context;
    if (adapter == NULL || !adapter->initialized || !adapter->has_irq ||
        asserted == NULL) {
        return -1;
    }
    const int value = gpio_pin_get_dt(&adapter->irq);
    if (value < 0) {
        return -1;
    }
    /* gpio_pin_get_dt() returns the logical value, including ACTIVE_LOW. */
    *asserted = value != 0;
    return 0;
}

int zap_zephyr_ztm_pwm_adapter_init(
    zap_zephyr_ztm_pwm_adapter_t *adapter,
    const zap_zephyr_ztm_pwm_adapter_config_t *config)
{
    if (adapter == NULL || config == NULL || config->i2c == NULL ||
        config->power.port == NULL || !gpio_is_ready_dt(&config->power) ||
        (config->irq != NULL && !gpio_is_ready_dt(config->irq))) {
        return ZAP_I2C_E_BAD_ARG;
    }

    adapter->initialized = false;
    adapter->power = config->power;
    adapter->has_irq = config->irq != NULL;
    if (adapter->has_irq) {
        adapter->irq = *config->irq;
    }

    int rc = zap_zephyr_i2c_init(&adapter->i2c,
                                 config->i2c,
                                 config->i2c_timeout_us,
                                 config->thread_safe);
    if (rc != ZAP_I2C_OK) {
        return rc;
    }
    zap_zephyr_time_init(&adapter->time);

    if (gpio_pin_configure_dt(&adapter->power, GPIO_OUTPUT_INACTIVE) != 0) {
        return ZAP_I2C_E_IO;
    }
    adapter->power_enabled = false;
    if (adapter->has_irq &&
        gpio_pin_configure_dt(&adapter->irq, GPIO_INPUT) != 0) {
        return ZAP_I2C_E_IO;
    }

    adapter->initialized = true;
    return ZAP_I2C_OK;
}

ztm_pwm_config_t zap_zephyr_ztm_pwm_make_config(
    zap_zephyr_ztm_pwm_adapter_t *adapter)
{
    ztm_pwm_config_t config = {0};
    if (adapter == NULL || !adapter->initialized) {
        return config;
    }

    config.i2c = zap_zephyr_i2c_bus(&adapter->i2c);
    config.time = &adapter->time;
    config.control_context = adapter;
    config.power_set = zephyr_power_set;
    config.power_get = zephyr_power_get;
    config.irq_read = adapter->has_irq ? zephyr_irq_read : NULL;
    return config;
}
