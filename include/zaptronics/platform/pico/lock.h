// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "pico/sync.h"
#include "zaptronics/platform/lock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mutex_t mutex;
    zap_lock_t lock;
} zap_pico_lock_t;

void zap_pico_lock_init(zap_pico_lock_t *adapter, uint32_t timeout_us);
const zap_lock_t *zap_pico_lock(const zap_pico_lock_t *adapter);

#ifdef __cplusplus
}
#endif
