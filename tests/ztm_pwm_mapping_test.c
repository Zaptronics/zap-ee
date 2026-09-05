// SPDX-License-Identifier: Apache-2.0
#include <zaptronics/modules/ztm_pwm.h>

int main(void)
{
    if (ztm_pwm_channel_get_sx_pin(1u) == ztm_pwm_channel_get_sx_pin(2u)) {
        return 1;
    }
    for (uint8_t fan = 1u; fan <= ZTM_PWM_FAN_COUNT; ++fan) {
        uint8_t emc = 0u;
        uint8_t local = 0u;
        if (!ztm_pwm_fan_get_route(fan, &emc, &local) || emc == 0u || local == 0u) {
            return 2;
        }
    }
    return ztm_pwm_gpio_get_sx_pin(1u) <= 15u ? 0 : 3;
}
