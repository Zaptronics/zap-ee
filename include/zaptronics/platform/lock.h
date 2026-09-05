// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Optional operation-level lock. A NULL lock means the caller guarantees
 * single-threaded access. timeout_us == 0 requests a non-blocking attempt.
 */
typedef struct {
    void *context;
    int (*acquire)(void *context, uint32_t timeout_us);
    void (*release)(void *context);
    uint32_t timeout_us;
} zap_lock_t;

static inline bool zap_lock_valid(const zap_lock_t *lock)
{
    return lock == NULL || (lock->acquire != NULL && lock->release != NULL);
}

static inline int zap_lock_acquire(const zap_lock_t *lock)
{
    if (lock == NULL) {
        return 0;
    }
    if (!zap_lock_valid(lock)) {
        return -1;
    }
    return lock->acquire(lock->context, lock->timeout_us);
}

static inline int zap_lock_try_acquire(const zap_lock_t *lock)
{
    if (lock == NULL) {
        return 0;
    }
    if (!zap_lock_valid(lock)) {
        return -1;
    }
    return lock->acquire(lock->context, 0u);
}

static inline void zap_lock_release(const zap_lock_t *lock)
{
    if (lock != NULL && zap_lock_valid(lock)) {
        lock->release(lock->context);
    }
}

#ifdef __cplusplus
}
#endif
