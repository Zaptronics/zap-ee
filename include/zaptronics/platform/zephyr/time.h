// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zaptronics/platform/time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fill a ZapEE time provider using Zephyr k_msleep(). */
void zap_zephyr_time_init(zap_time_t *time);

#ifdef __cplusplus
}
#endif
