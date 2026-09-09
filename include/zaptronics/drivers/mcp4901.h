// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zaptronics/platform/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCP4901_SPI_BAUD_HZ 1000000u

typedef enum {
    MCP4901_OK = 0,
    MCP4901_E_BAD_ARG = -1,
    MCP4901_E_SPI = -2,
} mcp4901_result_t;

typedef struct {
    const zap_spi_device_t *spi;
    bool ref_buffered;
    bool gain_1x;
} mcp4901_t;

int mcp4901_init(mcp4901_t *dev,
                 const zap_spi_device_t *spi,
                 bool ref_buffered,
                 bool gain_1x);
int mcp4901_set_raw(mcp4901_t *dev, uint8_t code);
int mcp4901_shutdown(mcp4901_t *dev);
int mcp4901_set_millivolts(mcp4901_t *dev,
                           uint32_t vref_mV,
                           uint32_t vout_mV,
                           uint8_t *written_code);

/* Pure frame helper, useful to chain composers and host-side tests. */
uint16_t mcp4901_make_frame(uint8_t code,
                            bool ref_buffered,
                            bool gain_1x,
                            bool active);

#ifdef __cplusplus
}
#endif
