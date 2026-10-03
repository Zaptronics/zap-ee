// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zaptronics/boards/ztpico2/connector.h"
#include "zaptronics/modules/ztm_psu_tec.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One persistent binding per module. Do not copy or move after initialization:
 * the module stores pointers into this object. */
typedef struct {
    ztmb_zap_spi_t spi;
    ztmb_zap_lock_t lock;
    ztpico2_connector_t connector;
} ztpico2_ztm_psu_tec_t;

typedef struct {
    ztpico2_t *board;
    spi_inst_t *spi;
    uint cs_mux_id;      /* Decoded CS line, 0..15. */
    int8_t enable_pin;   /* PCAL6416 enable output, 0..15. */
    bool ref_buffered;
    bool gain_1x;
} ztpico2_ztm_psu_tec_config_t;

/* Board must already be active and ready. Caller owns both binding and module;
 * their storage and the board must outlive every module operation.
 * Use normal ztm_psu_tec_* APIs after initialization, including deinit.
 * Init/reinit requires quiescent users and an inactive module; never rebind a
 * live module. Select distinct physical CS/enable endpoints for each instance.
 * Returns -1 for bad arguments, -2 for connector preparation failure, -3 for
 * transport setup failure, otherwise the result from ztm_psu_tec_init.
 * On any failure the module must not be used for normal operations. If module
 * initialization was reached, its state is retained for deinit/recovery.
 */
int ztpico2_ztm_psu_tec_init(ztpico2_ztm_psu_tec_t *binding,
                           ztm_psu_tec_t *module,
                           const ztpico2_ztm_psu_tec_config_t *config);

#ifdef __cplusplus
}
#endif
