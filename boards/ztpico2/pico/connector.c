#include "zaptronics/boards/ztpico2/connector.h"
static bool valid(ztpico2_connector_t *c) { return c && c->board && c->board == ztpico2_active() && c->board->ready; }
int ztpico2_connector_enable_set(void *context, bool enabled)
{
    ztpico2_connector_t *c=context;
    return valid(c) && c->enable_pin >= 0 && pcal6416a_write_pin(&c->board->expander, (uint8_t)c->enable_pin, enabled) ? 0 : -1;
}
int ztpico2_connector_enable_get(void *context, bool *enabled)
{
    ztpico2_connector_t *c=context;
    return valid(c) && c->enable_pin >= 0 && enabled && pcal6416a_read_pin(&c->board->expander, (uint8_t)c->enable_pin, enabled) ? 0 : -1;
}
int ztpico2_connector_input_read(void *context, bool *asserted)
{
    ztpico2_connector_t *c=context;
    bool level;
    if (!valid(c) || !asserted || c->input_pin < 0 || !pcal6416a_read_pin(&c->board->expander, (uint8_t)c->input_pin, &level)) return -1;
    *asserted = c->input_active_low ? !level : level;
    return 0;
}
int ztpico2_connector_prepare(void *context)
{
    ztpico2_connector_t *c=context;
    if (!valid(c) || c->enable_pin < 0) return -1;
    if (ztpico2_connector_enable_set(c, false) != 0 ||
        !pcal6416a_set_pin_direction(&c->board->expander, (uint8_t)c->enable_pin, false)) return -1;
    return c->input_pin < 0 || pcal6416a_set_pin_direction(&c->board->expander, (uint8_t)c->input_pin, true) ? 0 : -1;
}
