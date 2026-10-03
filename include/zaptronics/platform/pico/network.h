#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "zaptronics/services/log.h"
typedef int (*cli_line_cb_t)(char *cmd, char *out, size_t out_len);
typedef struct {
    const unsigned char *index_html;
    size_t index_html_len;
    const unsigned char *app_js;
    size_t app_js_len;
} zap_http_assets_t;
typedef struct {
    const char *ssid;
    const char *password;
    const char *banner;
    const char *prompt;
    uint16_t cli_port;
    uint16_t http_port; /* zero disables HTTP */
    cli_line_cb_t command;
    const zap_log_t *log;
    zap_http_assets_t assets;
    /* Optional screen callbacks; all execute in the network callback context. */
    void (*key)(uint8_t byte);
    void (*screen_tick)(void);
    void (*screen_closed)(void);
} zap_pico_network_config_t;
/* One service instance per cyw43/lwIP stack. Config string/asset/logger storage
 * must outlive the server. Startup uses the existing blocking retry policy.
 * Command callbacks execute synchronously in the network callback context. */
int zap_pico_network_start(const zap_pico_network_config_t *config);
void zap_pico_network_write(const char *text);
/* Enter raw mode only from a TCP CLI command (never HTTP). Network context only.
 * The client must disable local line editing. Raw mode suppresses prompts. */
bool zap_pico_network_set_raw(bool enabled);
bool zap_pico_network_output_idle(void);
bool zap_pico_network_status(char *buffer, size_t capacity);
