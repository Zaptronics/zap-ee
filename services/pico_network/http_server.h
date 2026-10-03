#pragma once
#include "zaptronics/platform/pico/network.h"
bool zap_pico_http_start(cli_line_cb_t command, uint16_t port, const zap_http_assets_t *assets);
