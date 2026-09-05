// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>
#include <zephyr/kernel.h>
#include "zaptronics/platform/lock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct k_mutex mutex;
    zap_lock_t lock;
} zap_zephyr_lock_t;

void zap_zephyr_lock_init(zap_zephyr_lock_t *adapter, uint32_t timeout_us);
const zap_lock_t *zap_zephyr_lock(const zap_zephyr_lock_t *adapter);

#ifdef __cplusplus
}
#endif
