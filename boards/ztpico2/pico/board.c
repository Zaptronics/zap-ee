// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/boards/ztpico2/board.h"
#include "zaptronics/boards/ztpico2/pins.h"
#include "pico/stdlib.h"
#include <limits.h>
#include <string.h>

static ztpico2_t *active_board;
ztpico2_t *ztpico2_active(void) { return active_board; }
static int stage_adc_mode(void *context, uint8_t mode)
{
    ztmb_zap_framed_spi_t *spi = context;
    return bus_spi_set_mode(&spi->bus, (mode & 2u) ? SPI_CPOL_1 : SPI_CPOL_0,
                            (mode & 1u) ? SPI_CPHA_1 : SPI_CPHA_0) ? 0 : -1;
}
int ztpico2_init(ztpico2_t *board, const ztpico2_config_t *config)
{
    if (!board || (active_board && active_board != board)) return -1;
    if (active_board == board) return board->ready ? 0 : -1;
    memset(board, 0, sizeof(*board));
    active_board = board;
    board->stage = ZTPICO2_STAGE_BUSES;
    cs_init();

    // Initialise the physical SPI peripherals in a neutral state. The managed
    // bus layer applies each device's baud rate and mode before every transaction.
    spi_init(ZTMB_SPI0_PORT, 1000*1000);
    spi_set_format(ZTMB_SPI0_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    //spi_set_format(ZTMB_SPI0_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(ZTMB_SPI0_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ZTMB_SPI0_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(ZTMB_SPI0_MOSI, GPIO_FUNC_SPI);

    spi_init(ZTMB_SPI1_PORT, 1000*1000);
    spi_set_format(ZTMB_SPI1_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(ZTMB_SPI1_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ZTMB_SPI1_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(ZTMB_SPI1_MOSI, GPIO_FUNC_SPI);

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(ZTMB_I2C0_PORT, 400 * 1000);
    gpio_set_function(ZTMB_I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(ZTMB_I2C0_SCL, GPIO_FUNC_I2C);
    //gpio_pull_up(ZTMB_I2C0_SDA);
    //gpio_pull_up(ZTMB_I2C0_SCL);

    // --- I2C1 setup on GPIO 18/19 ---
    i2c_init(ZTMB_I2C1_PORT, 400 * 1000); // 400 kHz
    gpio_set_function(ZTMB_I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(ZTMB_I2C1_SCL_PIN, GPIO_FUNC_I2C);
    //gpio_pull_up(ZTMB_I2C1_SDA_PIN);
    //gpio_pull_up(ZTMB_I2C1_SCL_PIN);
    // --- INT pin (active LOW, open-drain from PCAL6416A) ---
    gpio_init(ZTMB_EX_INT);
    gpio_set_dir(ZTMB_EX_INT, GPIO_IN);
    gpio_pull_up(ZTMB_EX_INT); // if you don't have an external pull-up

    if (!bus_init()) {
        return -2;
    }

    board->stage = ZTPICO2_STAGE_EXPANDER;
    if (!ztmb_zap_i2c_init(&board->expander_i2c, ZTMB_I2C1_PORT, 400000u, 5000u)) return -3;
    ztmb_zap_lock_init(&board->expander_lock, 5000u);
    if (!pcal6416a_init(&board->expander, &board->expander_i2c.bus, PCAL6416A_ADDR_BASE,
                        ZTPICO2_EXPANDER_INPUTS, ZTPICO2_EXPANDER_OUTPUTS,
                        ztmb_zap_lock(&board->expander_lock))) return -3;
    board->stage = ZTPICO2_STAGE_ADC;
    const bus_spi_config_t adc_config = {
        .baud_hz=1000000u, .data_bits=8, .cpol=SPI_CPOL_0, .cpha=SPI_CPHA_0,
        .order=SPI_MSB_FIRST, .cs_setup_us=0, .cs_hold_us=0, .cs_high_us=1
    };
    if (!ztmb_zap_framed_spi_init(&board->adc_spi, ZTMB_SPI1_PORT, ZTMB_ADC_CS_1,
                                 EX_NONE, &adc_config, 5000u, 0u)) return -4;
    ztmb_zap_lock_init(&board->adc_lock, 500000u);
    board->adc.vref_mv = config && config->adc_reference_mv ? config->adc_reference_mv : ZTPICO2_ADC_REFERENCE_MV;
    if (tla2518_init(&board->adc, &board->adc_spi.device, stage_adc_mode, &board->adc_spi) != 0) return -4;
    board->stage = ZTPICO2_STAGE_READY;
    board->ready = true;
    return 0;
}
bool ztpico2_deinit(ztpico2_t *board)
{
    if (!board || active_board != board) return false;
    /* Module deinitialization belongs to the application and precedes this. */
    const uint8_t enables[] = {EX_TEC_EN_1, EX_TEC_EN_2, EX_TEC_EN_3, EX_TEC_EN_4,
        EX_STEP_EN_1, EX_SPI_ALT_EN_1, EX_SPI_ALT_EN_2, EX_I2C_ALT_EN_1, EX_HV_EN_1};
    bool ok = true;
    if (board->expander.i2c.bus) {
        for (size_t i=0; i<sizeof(enables)/sizeof(enables[0]); ++i)
            if (!pcal6416a_write_pin(&board->expander, enables[i], false)) ok=false;
    }
    cs_deselect();
    board->ready = false;
    if (ok) active_board = NULL;
    return ok;
}
bool ztpico2_adc_read(ztpico2_t *board, uint8_t channel,
                      uint32_t ratio_thousands, uint16_t *code, uint32_t *uV)
{
    if (!board || board != active_board || !board->ready || !uV || channel > 7u ||
        ratio_thousands == 0u || ratio_thousands > INT_MAX) return false;
    const zap_lock_t *lock = ztmb_zap_lock(&board->adc_lock);
    if (zap_lock_acquire(lock) != 0) return false;
    uint16_t sample=0;
    bool ok = tla2518_read_channel_manual(&board->adc, channel, &sample, 50u) == 0;
    if (ok) {
        if (code) *code=sample;
        uint64_t scaled=(uint64_t)tla2518_code_to_uV(&board->adc, sample) * ratio_thousands / 1000u;
        if (scaled > UINT32_MAX) ok=false;
        else *uV=(uint32_t)scaled;
    }
    zap_lock_release(lock);
    return ok;
}
