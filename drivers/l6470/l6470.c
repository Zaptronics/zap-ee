// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/drivers/l6470.h"

bool l6470_status_has_stall(uint16_t status)
{
    return (status & L6470_STATUS_STALL_MASK) != L6470_STATUS_STALL_MASK;
}

bool l6470_status_has_thermal_warning(uint16_t status)
{
    return (status & L6470_STATUS_TH_WRN) == 0u;
}

bool l6470_status_has_thermal_shutdown(uint16_t status)
{
    return (status & L6470_STATUS_TH_SD) == 0u;
}

bool l6470_status_has_over_temp(uint16_t status)
{
    return l6470_status_has_thermal_warning(status) ||
           l6470_status_has_thermal_shutdown(status);
}

void l6470_encode_run(bool dir_fwd, uint32_t speed_20bit,
                      uint8_t frames[L6470_RUN_FRAME_COUNT])
{
    if (frames == NULL) return;
    if (speed_20bit > 0xFFFFFu) speed_20bit = 0xFFFFFu;
    frames[0] = (uint8_t)(L6470_CMD_RUN | (dir_fwd ? 1u : 0u));
    frames[1] = (uint8_t)(speed_20bit >> 16);
    frames[2] = (uint8_t)(speed_20bit >> 8);
    frames[3] = (uint8_t)speed_20bit;
}

void l6470_encode_move(bool dir_fwd, uint32_t nsteps_22bit,
                       uint8_t frames[L6470_MOVE_FRAME_COUNT])
{
    if (frames == NULL) return;
    if (nsteps_22bit > 0x3FFFFFu) nsteps_22bit = 0x3FFFFFu;
    frames[0] = (uint8_t)(L6470_CMD_MOVE | (dir_fwd ? 1u : 0u));
    frames[1] = (uint8_t)((nsteps_22bit >> 16) & 0x3Fu);
    frames[2] = (uint8_t)(nsteps_22bit >> 8);
    frames[3] = (uint8_t)nsteps_22bit;
}

size_t l6470_encode_set_param(uint8_t reg, uint32_t value, uint8_t nbytes,
                              uint8_t frames[L6470_MAX_SEQUENCE_BYTES])
{
    if (frames == NULL || nbytes == 0u || nbytes > 4u) return 0u;
    frames[0] = (uint8_t)(L6470_CMD_SET_PARAM | reg);
    for (uint8_t i = 0u; i < nbytes; ++i) {
        uint8_t shift = (uint8_t)(8u * (nbytes - 1u - i));
        frames[1u + i] = (uint8_t)(value >> shift);
    }
    return (size_t)nbytes + 1u;
}

uint32_t l6470_speed_from_steps_per_s(float sps)
{
    float v = sps * 67.108864f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1048575.0f) v = 1048575.0f;
    return (uint32_t)(v + 0.5f);
}

uint16_t l6470_acc_from_sps2(float sps2)
{
    float v = sps2 * 0.137438f;
    if (v < 0.0f) v = 0.0f;
    if (v > 4095.0f) v = 4095.0f;
    return (uint16_t)(v + 0.5f);
}

uint16_t l6470_max_speed_from_steps_per_s(float sps)
{
    float v = sps * 0.065536f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1023.0f) v = 1023.0f;
    return (uint16_t)(v + 0.5f);
}
