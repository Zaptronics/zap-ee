// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zaptronics/boards/ztpico2/board.h"
#include "zaptronics/modules/ztm_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Persistent caller-owned storage; do not copy or move after initialization. */
typedef struct {
    ztmb_zap_spi_t spi;
    ztmb_zap_lock_t lock;
} ztpico2_ztm_adc_t;

typedef struct {
    ztpico2_t *board;
    spi_inst_t *spi;
    uint cs_mux_id;    /* Decoded CS line, 0..15. */
    int8_t enable_pin; /* Endpoint metadata: -1 for none, otherwise 0..15.
                       * This adapter does not configure or toggle power. */
    void *event_context;
    ztm_adc_event_fn event; /* Optional application diagnostics. */
} ztpico2_ztm_adc_config_t;

/* Requires an active, ready board and quiescent module users. Binding, module,
 * board and event context must outlive operations. Never rebind a live module.
 * Use normal ztm_adc_* APIs, including deinit, after initialization.
 * Returns -1 for bad arguments, -2 for transport failure, otherwise the
 * portable initializer's result (negative codes overlap). On failure, do not
 * perform normal operations. If module init was reached, retain its state for
 * deinit/recovery; earlier failures do not initialize the module. */
int ztpico2_ztm_adc_init(ztpico2_ztm_adc_t *binding,
                       ztm_adc_t *module,
                       const ztpico2_ztm_adc_config_t *config);

#ifdef __cplusplus
}
#endif
