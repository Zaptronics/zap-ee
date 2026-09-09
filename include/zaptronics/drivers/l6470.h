// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L6470_CMD_NOP          0x00u
#define L6470_CMD_GET_STATUS   0xD0u
#define L6470_CMD_RESET_DEVICE 0xC0u
#define L6470_CMD_SOFT_STOP    0xB0u
#define L6470_CMD_HARD_STOP    0xB8u
#define L6470_CMD_SOFT_HIZ     0xA0u
#define L6470_CMD_HARD_HIZ     0xA8u
#define L6470_CMD_SET_PARAM    0x00u
#define L6470_CMD_GET_PARAM    0x20u
#define L6470_CMD_RUN          0x50u
#define L6470_CMD_MOVE         0x40u
#define L6470_CMD_GOTO         0x60u
#define L6470_CMD_GOTO_DIR     0x68u
#define L6470_CMD_GO_HOME      0x70u
#define L6470_CMD_GO_MARK      0x78u
#define L6470_CMD_RESET_POS    0xD8u

#define L6470_REG_ABS_POS      0x01u
#define L6470_REG_EL_POS       0x02u
#define L6470_REG_MARK         0x03u
#define L6470_REG_SPEED        0x04u
#define L6470_REG_ACC          0x05u
#define L6470_REG_DEC          0x06u
#define L6470_REG_MAX_SPEED    0x07u
#define L6470_REG_MIN_SPEED    0x08u
#define L6470_REG_KVAL_HOLD    0x09u
#define L6470_REG_KVAL_RUN     0x0Au
#define L6470_REG_KVAL_ACC     0x0Bu
#define L6470_REG_KVAL_DEC     0x0Cu
#define L6470_REG_STEP_MODE    0x16u
#define L6470_REG_ALARM_EN     0x17u
#define L6470_REG_CONFIG       0x18u

#define L6470_STATUS_STEP_LOSS_B (1u << 14)
#define L6470_STATUS_STEP_LOSS_A (1u << 13)
#define L6470_STATUS_TH_SD       (1u << 11)
#define L6470_STATUS_TH_WRN      (1u << 10)
#define L6470_STATUS_STALL_MASK (L6470_STATUS_STEP_LOSS_A | L6470_STATUS_STEP_LOSS_B)

#define L6470_RUN_FRAME_COUNT  4u
#define L6470_MOVE_FRAME_COUNT 4u
#define L6470_MAX_SEQUENCE_BYTES 5u

bool l6470_status_has_stall(uint16_t status);
bool l6470_status_has_thermal_warning(uint16_t status);
bool l6470_status_has_thermal_shutdown(uint16_t status);
bool l6470_status_has_over_temp(uint16_t status);

void l6470_encode_run(bool dir_fwd, uint32_t speed_20bit,
                      uint8_t frames[L6470_RUN_FRAME_COUNT]);
void l6470_encode_move(bool dir_fwd, uint32_t nsteps_22bit,
                       uint8_t frames[L6470_MOVE_FRAME_COUNT]);
size_t l6470_encode_set_param(uint8_t reg, uint32_t value, uint8_t nbytes,
                              uint8_t frames[L6470_MAX_SEQUENCE_BYTES]);

uint32_t l6470_speed_from_steps_per_s(float sps);
uint16_t l6470_acc_from_sps2(float sps2);
uint16_t l6470_max_speed_from_steps_per_s(float sps);

#ifdef __cplusplus
}
#endif
