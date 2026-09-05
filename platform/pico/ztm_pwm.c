// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/pico/ztm_pwm.h"

#include "hardware/gpio.h"

static int pico_power_set(void *context, bool enabled)
{
    zap_pico_ztm_pwm_adapter_t *adapter =
        (zap_pico_ztm_pwm_adapter_t *)context;
    if (adapter == NULL || !adapter->initialized) {
        return -1;
    }
    const bool physical = enabled == adapter->power_active_high;
    gpio_put((uint)adapter->power_gpio, physical);
    adapter->power_enabled = enabled;
    return 0;
}

static int pico_power_get(void *context, bool *enabled)
{
    zap_pico_ztm_pwm_adapter_t *adapter =
        (zap_pico_ztm_pwm_adapter_t *)context;
    if (adapter == NULL || !adapter->initialized || enabled == NULL) {
        return -1;
    }
    *enabled = adapter->power_enabled;
    return 0;
}

static int pico_irq_read(void *context, bool *asserted)
{
    zap_pico_ztm_pwm_adapter_t *adapter =
        (zap_pico_ztm_pwm_adapter_t *)context;
    if (adapter == NULL || !adapter->initialized || asserted == NULL ||
        adapter->irq_gpio < 0) {
        return -1;
    }
    const bool physical = gpio_get((uint)adapter->irq_gpio) != 0;
    *asserted = adapter->irq_active_low ? !physical : physical;
    return 0;
}

int zap_pico_ztm_pwm_adapter_init(
    zap_pico_ztm_pwm_adapter_t *adapter,
    const zap_pico_ztm_pwm_adapter_config_t *config)
{
    if (adapter == NULL || config == NULL || config->i2c == NULL ||
        config->power_gpio >= NUM_BANK0_GPIOS ||
        config->irq_gpio >= (int32_t)NUM_BANK0_GPIOS ||
        config->irq_gpio < -1 ||
        config->irq_pull < ZAP_PICO_GPIO_PULL_NONE ||
        config->irq_pull > ZAP_PICO_GPIO_PULL_DOWN) {
        return ZAP_I2C_E_BAD_ARG;
    }

    adapter->initialized = false;
    adapter->power_gpio = config->power_gpio;
    adapter->irq_gpio = config->irq_gpio;
    adapter->power_active_high = config->power_active_high;
    adapter->irq_active_low = config->irq_active_low;

    int rc = zap_pico_i2c_init(&adapter->i2c,
                               config->i2c,
                               config->i2c_timeout_us,
                               config->thread_safe);
    if (rc != ZAP_I2C_OK) {
        return rc;
    }
    zap_pico_time_init(&adapter->time);

    gpio_init((uint)adapter->power_gpio);
    gpio_set_dir((uint)adapter->power_gpio, GPIO_OUT);
    gpio_put((uint)adapter->power_gpio, !adapter->power_active_high);
    adapter->power_enabled = false;

    if (adapter->irq_gpio >= 0) {
        const uint irq_gpio = (uint)adapter->irq_gpio;
        gpio_init(irq_gpio);
        gpio_set_dir(irq_gpio, GPIO_IN);
        switch (config->irq_pull) {
            case ZAP_PICO_GPIO_PULL_UP:
                gpio_pull_up(irq_gpio);
                break;
            case ZAP_PICO_GPIO_PULL_DOWN:
                gpio_pull_down(irq_gpio);
                break;
            default:
                gpio_disable_pulls(irq_gpio);
                break;
        }
    }

    adapter->initialized = true;
    return ZAP_I2C_OK;
}

ztm_pwm_config_t zap_pico_ztm_pwm_make_config(
    zap_pico_ztm_pwm_adapter_t *adapter)
{
    ztm_pwm_config_t config = {0};
    if (adapter == NULL || !adapter->initialized) {
        return config;
    }

    config.i2c = zap_pico_i2c_bus(&adapter->i2c);
    config.time = &adapter->time;
    config.control_context = adapter;
    config.power_set = pico_power_set;
    config.power_get = pico_power_get;
    config.irq_read = adapter->irq_gpio >= 0 ? pico_irq_read : NULL;
    return config;
}
