#pragma once
#include "zaptronics/boards/ztpico2/zap_adapter.h"
#include "zaptronics/drivers/pcal6416.h"
#include "zaptronics/drivers/tla2518.h"

typedef enum {
    ZTPICO2_STAGE_NONE, ZTPICO2_STAGE_BUSES, ZTPICO2_STAGE_EXPANDER,
    ZTPICO2_STAGE_ADC, ZTPICO2_STAGE_READY
} ztpico2_stage_t;
typedef struct {
    uint16_t adc_reference_mv;
} ztpico2_config_t;
typedef struct {
    pcal6416a_t expander;
    tla2518_dev_t adc;
    ztmb_zap_i2c_t expander_i2c;
    ztmb_zap_framed_spi_t adc_spi;
    ztmb_zap_lock_t adc_lock;
    ztmb_zap_lock_t expander_lock;
    ztpico2_stage_t stage;
    bool ready;
} ztpico2_t;

/* One physical motherboard per MCU. Storage is caller-owned and must outlive
 * all endpoints. Init/deinit require quiescent module/command users. */
int ztpico2_init(ztpico2_t *board, const ztpico2_config_t *config);
bool ztpico2_deinit(ztpico2_t *board);
ztpico2_t *ztpico2_active(void);
bool ztpico2_adc_read(ztpico2_t *board, uint8_t channel,
                      uint32_t ratio_thousands, uint16_t *code, uint32_t *uV);
