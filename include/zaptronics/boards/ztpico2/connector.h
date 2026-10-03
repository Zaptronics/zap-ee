#pragma once
#include "zaptronics/boards/ztpico2/board.h"
typedef struct {
    ztpico2_t *board;
    int8_t enable_pin;
    int8_t input_pin;
    bool input_active_low;
} ztpico2_connector_t;
int ztpico2_connector_enable_set(void *context, bool enabled);
int ztpico2_connector_enable_get(void *context, bool *enabled);
int ztpico2_connector_input_read(void *context, bool *asserted);
int ztpico2_connector_prepare(void *context);
