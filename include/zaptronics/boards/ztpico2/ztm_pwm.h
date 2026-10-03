// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zaptronics/boards/ztpico2/connector.h"
#include "zaptronics/modules/ztm_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One persistent binding per module. Do not copy or move after initialization. */
typedef struct {
    ztmb_zap_i2c_t i2c;
    ztpico2_connector_t connector;
} ztpico2_ztm_pwm_t;

typedef struct {
    ztpico2_t *board;
    i2c_inst_t *i2c;
    int8_t enable_pin; /* PCAL6416 output, 0..15. */
    int8_t irq_pin;    /* PCAL6416 input, 0..15; -1 disables IRQ callback. */
    bool irq_active_low;
    void *event_context;
    ztm_pwm_event_fn event; /* Optional application event sink. */
} ztpico2_ztm_pwm_config_t;

/* Requires an active, ready board and quiescent module users.
 * Binding, module, board and event context must outlive module operations.
 * Never rebind a live module. Use ztm_pwm_* operations, including deinit,
 * after initialization. Multiple bindings do not resolve fixed I2C address
 * collisions: each module needs an electrically independent address domain.
 * Returns -1 for bad arguments, -2 for connector preparation failure, -3 for
 * transport setup failure, otherwise ztm_pwm_init's result (codes overlap).
 * A failed init does not permit normal operations. If module init was reached,
 * its state is retained for deinit/recovery; early failures do not initialize it.
 */
int ztpico2_ztm_pwm_init(ztpico2_ztm_pwm_t *binding,
                       ztm_pwm_t *module,
                       const ztpico2_ztm_pwm_config_t *config);

#ifdef __cplusplus
}
#endif
