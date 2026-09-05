// SPDX-License-Identifier: Apache-2.0
#include <math.h>

#include "zaptronics/modules/ztm_adc.h"

static bool close_enough(double a, double b, double tolerance)
{
    return fabs(a - b) <= tolerance;
}

int main(void)
{
    double temperature = 0.0;
    if (!ztm_adc_pt1000_temperature(1000.0, &temperature) ||
        !close_enough(temperature, 0.0, 0.001)) {
        return 1;
    }

    if (!ztm_adc_pt1000_temperature(1385.055, &temperature) ||
        !close_enough(temperature, 100.0, 0.02)) {
        return 2;
    }

    if (!close_enough(ztm_adc_rtd_resistance_from_code(0x800000u, 2000.0),
                      1000.0,
                      0.001)) {
        return 3;
    }

    double emf_mv = 0.0;
    if (!thermocouple_emf_mv(THERMOCOUPLE_TYPE_K, 0.0, &emf_mv) ||
        !close_enough(emf_mv, 0.0, 1e-9)) {
        return 4;
    }

    if (!thermocouple_compensated_temperature_c(THERMOCOUPLE_TYPE_K,
                                                 0.0,
                                                 25.0,
                                                 &temperature,
                                                 NULL) ||
        !close_enough(temperature, 25.0, 0.001)) {
        return 5;
    }

    return 0;
}
