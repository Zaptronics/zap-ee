// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/gpio.h>

#include "zaptronics/modules/ztm_pwm.h"
#include "zaptronics/platform/zephyr/i2c.h"
#include "zaptronics/platform/zephyr/time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const struct device *i2c;
    uint32_t i2c_timeout_us;
    bool thread_safe;
    struct gpio_dt_spec power;
    const struct gpio_dt_spec *irq; /* NULL when module IRQ is not connected. */
} zap_zephyr_ztm_pwm_adapter_config_t;

typedef struct {
    zap_zephyr_i2c_t i2c;
    zap_time_t time;
    struct gpio_dt_spec power;
    struct gpio_dt_spec irq;
    bool has_irq;
    bool power_enabled;
    bool initialized;
} zap_zephyr_ztm_pwm_adapter_t;

/*
 * Configures the enable pin as inactive output and the optional IRQ pin as
 * input. GPIO_ACTIVE_LOW/HIGH and pull flags come from each gpio_dt_spec.
 */
int zap_zephyr_ztm_pwm_adapter_init(
    zap_zephyr_ztm_pwm_adapter_t *adapter,
    const zap_zephyr_ztm_pwm_adapter_config_t *config);

ztm_pwm_config_t zap_zephyr_ztm_pwm_make_config(
    zap_zephyr_ztm_pwm_adapter_t *adapter);

#ifdef __cplusplus
}
#endif
