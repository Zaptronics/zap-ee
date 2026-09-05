// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *context;
    void (*delay_ms)(void *context, uint32_t delay_ms);
} zap_time_t;

static inline bool zap_time_valid(const zap_time_t *time)
{
    return time != NULL && time->delay_ms != NULL;
}

static inline void zap_delay_ms(const zap_time_t *time, uint32_t delay_ms)
{
    if (zap_time_valid(time)) {
        time->delay_ms(time->context, delay_ms);
    }
}

#ifdef __cplusplus
}
#endif
