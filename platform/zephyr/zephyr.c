// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/zephyr/i2c.h"
#include "zaptronics/platform/zephyr/lock.h"
#include "zaptronics/platform/zephyr/spi.h"
#include "zaptronics/platform/zephyr/time.h"

#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

static int zephyr_i2c_write(void *context,
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

static int zephyr_i2c_write_read(void *context,
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

static int zephyr_i2c_lock(void *context, uint32_t timeout_us)
{
    zap_zephyr_i2c_t *adapter = (zap_zephyr_i2c_t *)context;
    if (adapter == NULL) {
        return -1;
    }
    if (!adapter->mutex_enabled) {
        return 0;
    }
    const uint32_t timeout_ms = (timeout_us + 999u) / 1000u;
    return k_mutex_lock(&adapter->mutex,
                        timeout_us == 0u ? K_NO_WAIT : K_MSEC(timeout_ms)) == 0 ? 0 : -1;
}

static void zephyr_i2c_unlock(void *context)
{
    zap_zephyr_i2c_t *adapter = (zap_zephyr_i2c_t *)context;
    if (adapter != NULL && adapter->mutex_enabled) {
        (void)k_mutex_unlock(&adapter->mutex);
    }
}

static const zap_i2c_bus_ops_t g_zephyr_i2c_ops = {
    .write = zephyr_i2c_write,
    .write_read = zephyr_i2c_write_read,
    .probe = NULL,
    .lock = zephyr_i2c_lock,
    .unlock = zephyr_i2c_unlock,
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

static int zephyr_operation_lock(void *context, uint32_t timeout_us)
{
    zap_zephyr_lock_t *adapter = (zap_zephyr_lock_t *)context;
    if (adapter == NULL) {
        return -1;
    }
    const uint32_t timeout_ms = (timeout_us + 999u) / 1000u;
    return k_mutex_lock(&adapter->mutex,
                        timeout_us == 0u ? K_NO_WAIT : K_MSEC(timeout_ms)) == 0 ? 0 : -1;
}

static void zephyr_operation_unlock(void *context)
{
    zap_zephyr_lock_t *adapter = (zap_zephyr_lock_t *)context;
    if (adapter != NULL) {
        (void)k_mutex_unlock(&adapter->mutex);
    }
}

void zap_zephyr_lock_init(zap_zephyr_lock_t *adapter, uint32_t timeout_us)
{
    if (adapter == NULL) {
        return;
    }
    (void)k_mutex_init(&adapter->mutex);
    adapter->lock.context = adapter;
    adapter->lock.acquire = zephyr_operation_lock;
    adapter->lock.release = zephyr_operation_unlock;
    adapter->lock.timeout_us = timeout_us;
}

const zap_lock_t *zap_zephyr_lock(const zap_zephyr_lock_t *adapter)
{
    return adapter != NULL ? &adapter->lock : NULL;
}

static int zephyr_spi_begin(void *context, uint32_t timeout_us)
{
    (void)timeout_us;
    zap_zephyr_spi_t *adapter = (zap_zephyr_spi_t *)context;
    if (adapter == NULL || adapter->active) {
        return ZAP_SPI_E_STATE;
    }
    adapter->active = true;
    return ZAP_SPI_OK;
}

static int zephyr_spi_transfer_once(zap_zephyr_spi_t *adapter,
                                    const uint8_t *tx,
                                    uint8_t *rx,
                                    size_t len)
{
    struct spi_buf tx_buf = { .buf = (void *)tx, .len = len };
    struct spi_buf rx_buf = { .buf = rx, .len = len };
    struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1u };
    struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1u };
    const struct spi_buf_set *tx_ptr = tx != NULL ? &tx_set : NULL;
    const struct spi_buf_set *rx_ptr = rx != NULL ? &rx_set : NULL;
    return spi_transceive(adapter->spec.bus,
                          &adapter->spec.config,
                          tx_ptr,
                          rx_ptr);
}

static int zephyr_spi_transfer(void *context,
                               const uint8_t *tx,
                               uint8_t *rx,
                               size_t len,
                               uint8_t read_fill,
                               uint32_t timeout_us)
{
    (void)timeout_us;
    zap_zephyr_spi_t *adapter = (zap_zephyr_spi_t *)context;
    if (adapter == NULL || !adapter->active || len == 0u ||
        (tx == NULL && rx == NULL)) {
        return ZAP_SPI_E_BAD_ARG;
    }

    if (tx != NULL || read_fill == 0u) {
        return zephyr_spi_transfer_once(adapter, tx, rx, len) == 0
                   ? ZAP_SPI_OK
                   : ZAP_SPI_E_IO;
    }

    uint8_t fill[32];
    memset(fill, read_fill, sizeof(fill));
    size_t offset = 0u;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > sizeof(fill)) {
            chunk = sizeof(fill);
        }
        if (zephyr_spi_transfer_once(adapter,
                                     fill,
                                     rx != NULL ? &rx[offset] : NULL,
                                     chunk) != 0) {
            return ZAP_SPI_E_IO;
        }
        offset += chunk;
    }
    return ZAP_SPI_OK;
}

static int zephyr_spi_end(void *context)
{
    zap_zephyr_spi_t *adapter = (zap_zephyr_spi_t *)context;
    if (adapter == NULL || !adapter->active) {
        return ZAP_SPI_E_STATE;
    }
    adapter->active = false;
    return spi_release(adapter->spec.bus, &adapter->spec.config) == 0
               ? ZAP_SPI_OK
               : ZAP_SPI_E_IO;
}

static const zap_spi_device_ops_t g_zephyr_spi_ops = {
    .begin = zephyr_spi_begin,
    .transfer = zephyr_spi_transfer,
    .end = zephyr_spi_end,
};

int zap_zephyr_spi_init(zap_zephyr_spi_t *adapter,
                        const struct spi_dt_spec *spec,
                        uint32_t timeout_us,
                        uint8_t read_fill)
{
    if (adapter == NULL || spec == NULL || !spi_is_ready_dt(spec)) {
        return ZAP_SPI_E_BAD_ARG;
    }
    adapter->spec = *spec;
    adapter->spec.config.operation |= SPI_LOCK_ON | SPI_HOLD_ON_CS;
    adapter->active = false;
    adapter->device.context = adapter;
    adapter->device.ops = &g_zephyr_spi_ops;
    adapter->device.timeout_us = timeout_us;
    adapter->device.read_fill = read_fill;
    return ZAP_SPI_OK;
}

const zap_spi_device_t *zap_zephyr_spi_device(const zap_zephyr_spi_t *adapter)
{
    return adapter != NULL ? &adapter->device : NULL;
}

static void zephyr_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    (void)k_msleep((int32_t)delay_ms);
}

static void zephyr_delay_us(void *context, uint32_t delay_us)
{
    (void)context;
    k_busy_wait(delay_us);
}

static uint64_t zephyr_now_us(void *context)
{
    (void)context;
    return k_ticks_to_us_floor64(k_uptime_ticks());
}

void zap_zephyr_time_init(zap_time_t *time)
{
    if (time == NULL) {
        return;
    }
    time->context = NULL;
    time->delay_ms = zephyr_delay_ms;
    time->delay_us = zephyr_delay_us;
    time->now_us = zephyr_now_us;
}
