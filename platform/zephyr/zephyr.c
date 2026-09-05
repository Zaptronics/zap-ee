// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/zephyr/i2c.h"
#include "zaptronics/platform/zephyr/time.h"

#include <zephyr/drivers/i2c.h>

static int zephyr_write(void *context,
                        uint8_t address_7bit,
                        const uint8_t *data,
                        size_t len,
                        uint32_t timeout_us)
{
    (void)timeout_us;
    zap_zephyr_i2c_t *adapter = (zap_zephyr_i2c_t *)context;
    if (adapter == NULL || adapter->device == NULL || data == NULL || len == 0u) {
        return -1;
    }
    return i2c_write(adapter->device, data, len, address_7bit) == 0 ? 0 : -1;
}

static int zephyr_write_read(void *context,
                             uint8_t address_7bit,
                             const uint8_t *write_data,
                             size_t write_len,
                             uint8_t *read_data,
                             size_t read_len,
                             uint32_t timeout_us)
{
    (void)timeout_us;
    zap_zephyr_i2c_t *adapter = (zap_zephyr_i2c_t *)context;
    if (adapter == NULL || adapter->device == NULL ||
        write_data == NULL || write_len == 0u ||
        read_data == NULL || read_len == 0u) {
        return -1;
    }
    return i2c_write_read(adapter->device,
                          address_7bit,
                          write_data,
                          write_len,
                          read_data,
                          read_len) == 0
               ? 0
               : -1;
}

static int zephyr_lock(void *context, uint32_t timeout_us)
{
    zap_zephyr_i2c_t *adapter = (zap_zephyr_i2c_t *)context;
    if (adapter == NULL) {
        return -1;
    }
    if (!adapter->mutex_enabled) {
        return 0;
    }
    const uint32_t timeout_ms = (timeout_us + 999u) / 1000u;
    return k_mutex_lock(&adapter->mutex, K_MSEC(timeout_ms)) == 0 ? 0 : -1;
}

static void zephyr_unlock(void *context)
{
    zap_zephyr_i2c_t *adapter = (zap_zephyr_i2c_t *)context;
    if (adapter != NULL && adapter->mutex_enabled) {
        (void)k_mutex_unlock(&adapter->mutex);
    }
}

static const zap_i2c_bus_ops_t g_zephyr_i2c_ops = {
    .write = zephyr_write,
    .write_read = zephyr_write_read,
    .probe = NULL,
    .lock = zephyr_lock,
    .unlock = zephyr_unlock,
};

int zap_zephyr_i2c_init(zap_zephyr_i2c_t *adapter,
                        const struct device *device,
                        uint32_t timeout_us,
                        bool thread_safe)
{
    if (adapter == NULL || device == NULL || !device_is_ready(device)) {
        return ZAP_I2C_E_BAD_ARG;
    }

    adapter->device = device;
    adapter->mutex_enabled = thread_safe;
    if (thread_safe) {
        (void)k_mutex_init(&adapter->mutex);
    }
    adapter->bus.context = adapter;
    adapter->bus.ops = &g_zephyr_i2c_ops;
    adapter->bus.timeout_us = timeout_us;
    return ZAP_I2C_OK;
}

const zap_i2c_bus_t *zap_zephyr_i2c_bus(const zap_zephyr_i2c_t *adapter)
{
    return adapter != NULL ? &adapter->bus : NULL;
}

static void zephyr_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    (void)k_msleep((int32_t)delay_ms);
}

void zap_zephyr_time_init(zap_time_t *time)
{
    if (time == NULL) {
        return;
    }
    time->context = NULL;
    time->delay_ms = zephyr_delay_ms;
}
