// SPDX-License-Identifier: Apache-2.0
#include <math.h>

#include <zaptronics/modules/ztm_adc.h>
#include <zaptronics/modules/ztm_pwm.h>

int main(void)
{
    double temperature_c = 0.0;
    if (!ztm_adc_pt1000_temperature(1000.0, &temperature_c) ||
        fabs(temperature_c) > 0.001) {
        return 1;
    }
    if (ztm_pwm_channel_get_sx_pin(1u) >= 16u) {
        return 2;
    }
    return 0;
}
