// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zaptronics/platform/time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fill a ZapEE time provider using Pico SDK sleep_ms(). */
void zap_pico_time_init(zap_time_t *time);

#ifdef __cplusplus
}
#endif
