#pragma once
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "zaptronics/drivers/pcal6416.h"

#define ZTMB_SPI0_PORT spi0
#define ZTMB_SPI0_MISO 0
#define ZTMB_SPI0_SCK  2
#define ZTMB_SPI0_MOSI 3

#define ZTMB_SPI1_PORT spi1
#define ZTMB_SPI1_MISO 12
#define ZTMB_SPI1_SCK  10
#define ZTMB_SPI1_MOSI 11

#define ZTMB_I2C0_PORT i2c0
#define ZTMB_I2C0_SDA  8
#define ZTMB_I2C0_SCL  9

#define ZTMB_I2C1_PORT    i2c1
#define ZTMB_I2C1_SDA_PIN 18
#define ZTMB_I2C1_SCL_PIN 19
#define ZTMB_EX_INT       20


#define EX_NONE            (-1)
#define EX_TEC_EN_4         PCAL6416A_PIN_P0_0
#define EX_TEC_EN_3         PCAL6416A_PIN_P0_1
#define EX_TEC_EN_2         PCAL6416A_PIN_P0_2
#define EX_TEC_EN_1         PCAL6416A_PIN_P0_3
#define EX_STEP_FAULT       PCAL6416A_PIN_P0_4
#define EX_STEP_EN_1        PCAL6416A_PIN_P0_5
#define EX_SPI_ALT_GPIO_2   PCAL6416A_PIN_P0_6
#define EX_SPI_ALT_EN_2     PCAL6416A_PIN_P0_7
#define EX_SPI_ALT_GPIO_1   PCAL6416A_PIN_P1_0
#define EX_SPI_ALT_EN_1     PCAL6416A_PIN_P1_1
#define EX_I2C_ALT_IO_1     PCAL6416A_PIN_P1_2
#define EX_I2C_ALT_EN_1     PCAL6416A_PIN_P1_3
#define EX_I2C_ADDR0        PCAL6416A_PIN_P1_4
#define EX_HV_PGOOD_1       PCAL6416A_PIN_P1_5
#define EX_HV_EN_1          PCAL6416A_PIN_P1_6
#define EX_3V3_EN           PCAL6416A_PIN_P1_7



// First 74HC138 outputs (0..7)
#define ZTMB_SPI_ALT_CS_1   0
#define ZTMB_SPI_ALT_CS_2   1
#define ZTMB_STEP_CS_1      2
#define ZTMB_TEC_CS_1       4
#define ZTMB_TEC_CS_2       5
#define ZTMB_TEC_CS_3       6
#define ZTMB_TEC_CS_4       7

// Second 74HC138 outputs (8..15)
#define ZTMB_ADC_CS_2       8   // 24bit ZTM_ADC
#define ZTMB_ADC_SYNC       9
#define ZTMB_ADC_CS_1       14  // 12bit Onboard ADC

/* Existing board startup configuration: do not drive the P1.7 control. */
#define ZTPICO2_EXPANDER_INPUTS 0xA550u
#define ZTPICO2_EXPANDER_OUTPUTS 0x0000u
#define ZTPICO2_ADC_REFERENCE_MV 3300u
#define ZTPICO2_HV_SENSE_CHANNEL 1u
#define ZTPICO2_TEC1_SENSE_CHANNEL 5u
#define ZTPICO2_TEC2_SENSE_CHANNEL 4u
#define ZTPICO2_TEC3_SENSE_CHANNEL 3u
#define ZTPICO2_TEC4_SENSE_CHANNEL 2u
#define ZTPICO2_SENSE_DIVIDER_THOUSANDS 7500u

// ---- Pin mapping -----------------------------------------------------------

#ifndef CS_A0_PIN
#define CS_A0_PIN 6   // First 74HC138: A0
#endif

#ifndef CS_A1_PIN
#define CS_A1_PIN 5   // First 74HC138: A1
#endif

#ifndef CS_A2_PIN
#define CS_A2_PIN 4   // First 74HC138: A2
#endif

#ifndef CS_A3_PIN
#define CS_A3_PIN 26  // Second 74HC138: A0
#endif

#ifndef CS_A4_PIN
#define CS_A4_PIN 27  // Second 74HC138: A1
#endif

#ifndef CS_A5_PIN
#define CS_A5_PIN 28  // Second 74HC138: A2
#endif

// ---- Per-chip "none selected" addresses -----------------------------------
// For a 74HC138, address bits (A2,A1,A0) = 0,1,1 (binary 3) will select Y3.
// You said your unused / idle state is 0,1,1 for both chips.
//
// If you need to change this later, override before including this header:
//   #define CS0_NONE_ADDR  3
//   #define CS1_NONE_ADDR  3

#ifndef CS0_NONE_ADDR
#define CS0_NONE_ADDR 3
#endif

#ifndef CS1_NONE_ADDR
#define CS1_NONE_ADDR 3
#endif


/* Compatibility names for existing Hub diagnostics. */
#define ZTM_PSU_HV_VSENSE_CHANNEL ZTPICO2_HV_SENSE_CHANNEL
#define ZTM_PSU_HV_DIVIDER_RATIO_THOUSANDS ZTPICO2_SENSE_DIVIDER_THOUSANDS
