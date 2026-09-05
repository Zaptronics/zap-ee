// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/pico/i2c.h"
#include "zaptronics/platform/pico/time.h"

#include <limits.h>

#include "pico/time.h"

static int pico_write(void *context,
                      uint8_t address_7bit,
                      const uint8_t *data,
                      size_t len,
                      uint32_t timeout_us)
{
    zap_pico_i2c_t *adapter = (zap_pico_i2c_t *)context;
    if (adapter == NULL || adapter->instance == NULL || data == NULL ||
        len == 0u || len > (size_t)INT_MAX) {
        return -1;
    }

    const absolute_time_t until = make_timeout_time_us(timeout_us);
    const int rc = i2c_write_blocking_until(adapter->instance,
                                             address_7bit,
                                             data,
                                             len,
                                             false,
                                             until);
    return rc == (int)len ? 0 : -1;
}

static int pico_write_read(void *context,
                           uint8_t address_7bit,
                           const uint8_t *write_data,
                           size_t write_len,
                           uint8_t *read_data,
                           size_t read_len,
                           uint32_t timeout_us)
{
    zap_pico_i2c_t *adapter = (zap_pico_i2c_t *)context;
    if (adapter == NULL || adapter->instance == NULL ||
        write_data == NULL || write_len == 0u ||
        read_data == NULL || read_len == 0u ||
        write_len > (size_t)INT_MAX || read_len > (size_t)INT_MAX) {
        return -1;
    }

    /* One deadline covers both phases. nostop=true makes the read begin with
     * a repeated START, matching ZapEE's atomic register-read contract. */
    const absolute_time_t until = make_timeout_time_us(timeout_us);
    int rc = i2c_write_blocking_until(adapter->instance,
                                      address_7bit,
                                      write_data,
                                      write_len,
                                      true,
                                      until);
    if (rc != (int)write_len) {
        return -1;
    }

    rc = i2c_read_blocking_until(adapter->instance,
                                 address_7bit,
                                 read_data,
                                 read_len,
                                 false,
                                 until);
    return rc == (int)read_len ? 0 : -1;
}

static int pico_lock(void *context, uint32_t timeout_us)
{
    zap_pico_i2c_t *adapter = (zap_pico_i2c_t *)context;
    if (adapter == NULL) {
        return -1;
    }
    if (!adapter->mutex_enabled) {
        return 0;
    }
    return mutex_enter_timeout_us(&adapter->mutex, timeout_us) ? 0 : -1;
}

static void pico_unlock(void *context)
{
    zap_pico_i2c_t *adapter = (zap_pico_i2c_t *)context;
    if (adapter != NULL && adapter->mutex_enabled) {
        mutex_exit(&adapter->mutex);
    }
}

static const zap_i2c_bus_ops_t g_pico_i2c_ops = {
    .write = pico_write,
    .write_read = pico_write_read,
    .probe = NULL,
    .lock = pico_lock,
    .unlock = pico_unlock,
};

int zap_pico_i2c_init(zap_pico_i2c_t *adapter,
                      i2c_inst_t *instance,
                      uint32_t timeout_us,
                      bool thread_safe)
{
    if (adapter == NULL || instance == NULL) {
        return ZAP_I2C_E_BAD_ARG;
    }

    adapter->instance = instance;
    adapter->mutex_enabled = thread_safe;
    if (thread_safe) {
        mutex_init(&adapter->mutex);
    }

    adapter->bus.context = adapter;
    adapter->bus.ops = &g_pico_i2c_ops;
    adapter->bus.timeout_us = timeout_us;
    return ZAP_I2C_OK;
}

const zap_i2c_bus_t *zap_pico_i2c_bus(const zap_pico_i2c_t *adapter)
{
    return adapter != NULL ? &adapter->bus : NULL;
}

static void pico_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    sleep_ms(delay_ms);
}

void zap_pico_time_init(zap_time_t *time)
{
    if (time == NULL) {
        return;
    }
    time->context = NULL;
    time->delay_ms = pico_delay_ms;
}
