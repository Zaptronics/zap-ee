// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/drivers/mcp4901.h"

uint16_t mcp4901_make_frame(uint8_t code,
                            bool ref_buffered,
                            bool gain_1x,
                            bool active)
{
    uint16_t frame = 0u;
    frame |= (uint16_t)(ref_buffered ? 1u : 0u) << 14;
    frame |= (uint16_t)(gain_1x ? 1u : 0u) << 13;
    frame |= (uint16_t)(active ? 1u : 0u) << 12;
    frame |= (uint16_t)code << 4;
    return frame;
}

static int write_frame(mcp4901_t *dev, uint16_t frame)
{
    if (dev == NULL || !zap_spi_device_valid(dev->spi)) {
        return MCP4901_E_BAD_ARG;
    }

    const uint8_t bytes[2] = {
        (uint8_t)(frame >> 8),
        (uint8_t)(frame & 0xFFu),
    };
    return zap_spi_write(dev->spi, bytes, sizeof(bytes)) == ZAP_SPI_OK
               ? MCP4901_OK
               : MCP4901_E_SPI;
}

int mcp4901_init(mcp4901_t *dev,
                 const zap_spi_device_t *spi,
                 bool ref_buffered,
                 bool gain_1x)
{
    if (dev == NULL || !zap_spi_device_valid(spi)) {
        return MCP4901_E_BAD_ARG;
    }
    dev->spi = spi;
    dev->ref_buffered = ref_buffered;
    dev->gain_1x = gain_1x;
    return MCP4901_OK;
}

int mcp4901_set_raw(mcp4901_t *dev, uint8_t code)
{
    if (dev == NULL) {
        return MCP4901_E_BAD_ARG;
    }
    return write_frame(dev,
                       mcp4901_make_frame(code,
                                          dev->ref_buffered,
                                          dev->gain_1x,
                                          true));
}

int mcp4901_shutdown(mcp4901_t *dev)
{
    if (dev == NULL) {
        return MCP4901_E_BAD_ARG;
    }
    return write_frame(dev,
                       mcp4901_make_frame(0u,
                                          dev->ref_buffered,
                                          dev->gain_1x,
                                          false));
}

int mcp4901_set_millivolts(mcp4901_t *dev,
                           uint32_t vref_mV,
                           uint32_t vout_mV,
                           uint8_t *written_code)
{
    if (dev == NULL || vref_mV == 0u) {
        return MCP4901_E_BAD_ARG;
    }

    const uint32_t full_scale_mV = dev->gain_1x ? vref_mV : (2u * vref_mV);
    if (vout_mV >= full_scale_mV) {
        vout_mV = full_scale_mV - 1u;
    }

    const uint8_t code = (uint8_t)((vout_mV * 256u) / full_scale_mV);
    const int rc = mcp4901_set_raw(dev, code);
    if (rc != MCP4901_OK) {
        return rc;
    }
    if (written_code != NULL) {
        *written_code = code;
    }
    return MCP4901_OK;
}
