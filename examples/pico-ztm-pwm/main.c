// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "zaptronics/modules/ztm_pwm.h"
#include "zaptronics/platform/pico/ztm_pwm.h"

/* Example wiring only: change these to match the application. */
#define EXAMPLE_I2C i2c0
#define EXAMPLE_SDA_GPIO 4u
#define EXAMPLE_SCL_GPIO 5u
#define EXAMPLE_ENABLE_GPIO 10u
#define EXAMPLE_IRQ_GPIO 11

static void on_pwm_event(void *context, const ztm_pwm_event_t *event)
{
    (void)context;
    if (event != NULL) {
        printf("ZTM_PWM event=%u device=%u channel=%u detail=%lu\n",
               (unsigned)event->type,
               (unsigned)event->device_1based,
               (unsigned)event->channel_1based,
               (unsigned long)event->detail);
    }
}

int main(void)
{
    stdio_init_all();

    i2c_init(EXAMPLE_I2C, 400000u);
    gpio_set_function(EXAMPLE_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(EXAMPLE_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(EXAMPLE_SDA_GPIO);
    gpio_pull_up(EXAMPLE_SCL_GPIO);

    zap_pico_ztm_pwm_adapter_t platform = {0};
    const zap_pico_ztm_pwm_adapter_config_t platform_config = {
        .i2c = EXAMPLE_I2C,
        .i2c_timeout_us = 5000u,
        .thread_safe = true,
        .power_gpio = EXAMPLE_ENABLE_GPIO,
        .power_active_high = true,
        .irq_gpio = EXAMPLE_IRQ_GPIO,
        .irq_active_low = true,
        .irq_pull = ZAP_PICO_GPIO_PULL_UP,
    };

    if (zap_pico_ztm_pwm_adapter_init(&platform, &platform_config) != ZAP_I2C_OK) {
        panic("ZapEE Pico adapter init failed");
    }

    ztm_pwm_config_t pwm_config = zap_pico_ztm_pwm_make_config(&platform);
    pwm_config.event = on_pwm_event;

    ztm_pwm_t pwm = {0};
    if (ztm_pwm_init(&pwm, &pwm_config) != 0) {
        panic("ZTM_PWM init failed at %s (%d)",
              ztm_pwm_init_stage_name(pwm.last_init_stage),
              pwm.last_init_rc);
    }

    /* First useful operation: drive low-side PWM channel 1 at 25%. */
    (void)ztm_pwm_channel_set(&pwm, 1u, 25u);

    for (;;) {
        (void)ztm_pwm_poll(&pwm);
        sleep_ms(10u);
    }
}
