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
    void (*delay_us)(void *context, uint32_t delay_us);
    uint64_t (*now_us)(void *context);
} zap_time_t;

static inline bool zap_time_valid(const zap_time_t *time)
{
    return time != NULL && time->delay_ms != NULL;
}

static inline bool zap_time_high_resolution_valid(const zap_time_t *time)
{
    return zap_time_valid(time) && time->delay_us != NULL && time->now_us != NULL;
}

static inline void zap_delay_ms(const zap_time_t *time, uint32_t delay_ms)
{
    if (zap_time_valid(time)) {
        time->delay_ms(time->context, delay_ms);
    }
}

static inline void zap_delay_us(const zap_time_t *time, uint32_t delay_us)
{
    if (time == NULL) {
        return;
    }
    if (time->delay_us != NULL) {
        time->delay_us(time->context, delay_us);
    } else if (time->delay_ms != NULL) {
        time->delay_ms(time->context, (delay_us + 999u) / 1000u);
    }
}

static inline uint64_t zap_time_now_us(const zap_time_t *time)
{
    return time != NULL && time->now_us != NULL ? time->now_us(time->context) : 0u;
}

#ifdef __cplusplus
}
#endif
