// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/platform/pico/i2c.h"
#include "zaptronics/platform/pico/lock.h"
#include "zaptronics/platform/pico/spi.h"
#include "zaptronics/platform/pico/time.h"

#include <limits.h>

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/time.h"

static int pico_i2c_write(void *context,
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

static int pico_i2c_write_read(void *context,
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

static int pico_i2c_lock(void *context, uint32_t timeout_us)
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

static void pico_i2c_unlock(void *context)
{
    zap_pico_i2c_t *adapter = (zap_pico_i2c_t *)context;
    if (adapter != NULL && adapter->mutex_enabled) {
        mutex_exit(&adapter->mutex);
    }
}

static const zap_i2c_bus_ops_t g_pico_i2c_ops = {
    .write = pico_i2c_write,
    .write_read = pico_i2c_write_read,
    .probe = NULL,
    .lock = pico_i2c_lock,
    .unlock = pico_i2c_unlock,
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

static int pico_operation_lock(void *context, uint32_t timeout_us)
{
    zap_pico_lock_t *adapter = (zap_pico_lock_t *)context;
    if (adapter == NULL) {
        return -1;
    }
    if (timeout_us == 0u) {
        uint32_t owner = 0u;
        return mutex_try_enter(&adapter->mutex, &owner) ? 0 : -1;
    }
    return mutex_enter_timeout_us(&adapter->mutex, timeout_us) ? 0 : -1;
}

static void pico_operation_unlock(void *context)
{
    zap_pico_lock_t *adapter = (zap_pico_lock_t *)context;
    if (adapter != NULL) {
        mutex_exit(&adapter->mutex);
    }
}

void zap_pico_lock_init(zap_pico_lock_t *adapter, uint32_t timeout_us)
{
    if (adapter == NULL) {
        return;
    }
    mutex_init(&adapter->mutex);
    adapter->lock.context = adapter;
    adapter->lock.acquire = pico_operation_lock;
    adapter->lock.release = pico_operation_unlock;
    adapter->lock.timeout_us = timeout_us;
}

const zap_lock_t *zap_pico_lock(const zap_pico_lock_t *adapter)
{
    return adapter != NULL ? &adapter->lock : NULL;
}

static int pico_spi_begin(void *context, uint32_t timeout_us)
{
    zap_pico_spi_device_t *adapter = (zap_pico_spi_device_t *)context;
    if (adapter == NULL || adapter->bus == NULL || adapter->bus->instance == NULL ||
        adapter->active) {
        return ZAP_SPI_E_STATE;
    }

    if (adapter->bus->mutex_enabled &&
        !mutex_enter_timeout_us(&adapter->bus->mutex, timeout_us)) {
        return ZAP_SPI_E_LOCK;
    }

    spi_set_baudrate(adapter->bus->instance, adapter->config.baud_hz);
    spi_set_format(adapter->bus->instance,
                   adapter->config.data_bits,
                   adapter->config.cpol,
                   adapter->config.cpha,
                   adapter->config.order);
    gpio_put((uint)adapter->cs_gpio, 0);
    if (adapter->config.cs_setup_us != 0u) {
        busy_wait_us_32(adapter->config.cs_setup_us);
    }
    adapter->active = true;
    return ZAP_SPI_OK;
}

static int pico_spi_transfer(void *context,
                             const uint8_t *tx,
                             uint8_t *rx,
                             size_t len,
                             uint8_t read_fill,
                             uint32_t timeout_us)
{
    (void)timeout_us;
    zap_pico_spi_device_t *adapter = (zap_pico_spi_device_t *)context;
    if (adapter == NULL || adapter->bus == NULL || !adapter->active ||
        len == 0u || len > (size_t)INT_MAX || (tx == NULL && rx == NULL)) {
        return ZAP_SPI_E_BAD_ARG;
    }

    int rc;
    if (tx != NULL && rx != NULL) {
        rc = spi_write_read_blocking(adapter->bus->instance, tx, rx, len);
    } else if (tx != NULL) {
        rc = spi_write_blocking(adapter->bus->instance, tx, len);
    } else {
        rc = spi_read_blocking(adapter->bus->instance, read_fill, rx, len);
    }
    return rc == (int)len ? ZAP_SPI_OK : ZAP_SPI_E_IO;
}

static int pico_spi_end(void *context)
{
    zap_pico_spi_device_t *adapter = (zap_pico_spi_device_t *)context;
    if (adapter == NULL || adapter->bus == NULL || !adapter->active) {
        return ZAP_SPI_E_STATE;
    }

    if (adapter->config.cs_hold_us != 0u) {
        busy_wait_us_32(adapter->config.cs_hold_us);
    }
    gpio_put((uint)adapter->cs_gpio, 1);
    if (adapter->config.cs_high_us != 0u) {
        busy_wait_us_32(adapter->config.cs_high_us);
    }
    adapter->active = false;
    if (adapter->bus->mutex_enabled) {
        mutex_exit(&adapter->bus->mutex);
    }
    return ZAP_SPI_OK;
}

static const zap_spi_device_ops_t g_pico_spi_ops = {
    .begin = pico_spi_begin,
    .transfer = pico_spi_transfer,
    .end = pico_spi_end,
};

int zap_pico_spi_bus_init(zap_pico_spi_bus_t *bus,
                          spi_inst_t *instance,
                          bool thread_safe)
{
    if (bus == NULL || instance == NULL) {
        return ZAP_SPI_E_BAD_ARG;
    }
    bus->instance = instance;
    bus->mutex_enabled = thread_safe;
    if (thread_safe) {
        mutex_init(&bus->mutex);
    }
    return ZAP_SPI_OK;
}

int zap_pico_spi_device_init(zap_pico_spi_device_t *adapter,
                             zap_pico_spi_bus_t *bus,
                             uint32_t cs_gpio,
                             const zap_pico_spi_config_t *config,
                             uint32_t timeout_us,
                             uint8_t read_fill)
{
    if (adapter == NULL || bus == NULL || bus->instance == NULL || config == NULL ||
        cs_gpio >= NUM_BANK0_GPIOS || config->baud_hz == 0u ||
        config->data_bits < 4u || config->data_bits > 16u) {
        return ZAP_SPI_E_BAD_ARG;
    }

    adapter->bus = bus;
    adapter->cs_gpio = cs_gpio;
    adapter->config = *config;
    adapter->active = false;
    gpio_init((uint)cs_gpio);
    gpio_set_dir((uint)cs_gpio, GPIO_OUT);
    gpio_put((uint)cs_gpio, 1);

    adapter->device.context = adapter;
    adapter->device.ops = &g_pico_spi_ops;
    adapter->device.timeout_us = timeout_us;
    adapter->device.read_fill = read_fill;
    return ZAP_SPI_OK;
}

const zap_spi_device_t *zap_pico_spi_device(const zap_pico_spi_device_t *adapter)
{
    return adapter != NULL ? &adapter->device : NULL;
}

static void pico_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    sleep_ms(delay_ms);
}

static void pico_delay_us(void *context, uint32_t delay_us)
{
    (void)context;
    busy_wait_us_32(delay_us);
}

static uint64_t pico_now_us(void *context)
{
    (void)context;
    return time_us_64();
}

void zap_pico_time_init(zap_time_t *time)
{
    if (time == NULL) {
        return;
    }
    time->context = NULL;
    time->delay_ms = pico_delay_ms;
    time->delay_us = pico_delay_us;
    time->now_us = pico_now_us;
}
