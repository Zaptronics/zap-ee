// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zaptronics/modules/ztm_pwm.h"
#include "zaptronics/platform/pico/i2c.h"
#include "zaptronics/platform/pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ZAP_PICO_GPIO_PULL_NONE = 0,
    ZAP_PICO_GPIO_PULL_UP,
    ZAP_PICO_GPIO_PULL_DOWN,
} zap_pico_gpio_pull_t;

typedef struct {
    i2c_inst_t *i2c;
    uint32_t i2c_timeout_us;
    bool thread_safe;

    uint32_t power_gpio;
    bool power_active_high;

    /* Set irq_gpio to -1 when the module IRQ is not connected. */
    int32_t irq_gpio;
    bool irq_active_low;
    zap_pico_gpio_pull_t irq_pull;
} zap_pico_ztm_pwm_adapter_config_t;

typedef struct {
    zap_pico_i2c_t i2c;
    zap_time_t time;
    uint32_t power_gpio;
    int32_t irq_gpio;
    bool power_active_high;
    bool irq_active_low;
    bool power_enabled;
    bool initialized;
} zap_pico_ztm_pwm_adapter_t;

/*
 * Configure the module enable/IRQ GPIOs and prepare the portable ZapEE
 * transport/time providers. The I2C peripheral and SDA/SCL pins must already
 * have been configured by the application.
 */
int zap_pico_ztm_pwm_adapter_init(
    zap_pico_ztm_pwm_adapter_t *adapter,
    const zap_pico_ztm_pwm_adapter_config_t *config);

/*
 * Create a ready-to-use ZTM_PWM configuration. The caller may then set the
 * returned .event/.event_context fields before calling ztm_pwm_init().
 */
ztm_pwm_config_t zap_pico_ztm_pwm_make_config(
    zap_pico_ztm_pwm_adapter_t *adapter);

#ifdef __cplusplus
}
#endif
