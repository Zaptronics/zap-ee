// SPDX-License-Identifier: Apache-2.0
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "zaptronics/modules/ztm_pwm.h"
#include "zaptronics/platform/zephyr/ztm_pwm.h"

#define USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec g_power =
    GPIO_DT_SPEC_GET(USER_NODE, ztm_pwm_enable_gpios);
static const struct gpio_dt_spec g_irq =
    GPIO_DT_SPEC_GET(USER_NODE, ztm_pwm_irq_gpios);
static const struct device *const g_i2c = DEVICE_DT_GET(DT_NODELABEL(i2c0));

static void on_pwm_event(void *context, const ztm_pwm_event_t *event)
{
    (void)context;
    if (event != NULL) {
        printk("ZTM_PWM event=%u device=%u channel=%u detail=%u\n",
               event->type,
               event->device_1based,
               event->channel_1based,
               event->detail);
    }
}

int main(void)
{
    zap_zephyr_ztm_pwm_adapter_t platform = {0};
    const zap_zephyr_ztm_pwm_adapter_config_t platform_config = {
        .i2c = g_i2c,
        .i2c_timeout_us = 5000u,
        .thread_safe = true,
        .power = g_power,
        .irq = &g_irq,
    };

    if (zap_zephyr_ztm_pwm_adapter_init(&platform, &platform_config) != ZAP_I2C_OK) {
        printk("ZapEE Zephyr adapter init failed\n");
        return 0;
    }

    ztm_pwm_config_t pwm_config = zap_zephyr_ztm_pwm_make_config(&platform);
    pwm_config.event = on_pwm_event;

    ztm_pwm_t pwm = {0};
    if (ztm_pwm_init(&pwm, &pwm_config) != 0) {
        printk("ZTM_PWM init failed at %s (%d)\n",
               ztm_pwm_init_stage_name(pwm.last_init_stage),
               pwm.last_init_rc);
        return 0;
    }

    (void)ztm_pwm_channel_set(&pwm, 1u, 25u);
    while (true) {
        (void)ztm_pwm_poll(&pwm);
        k_msleep(10);
    }
    return 0;
}
