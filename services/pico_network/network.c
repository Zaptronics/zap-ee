#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/tcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"

#include "zaptronics/platform/pico/network.h"
#define RX_LINE_MAX 256
#define CLI_TX_BUFFER_MAX 16384
#include "http_server.h"
static zap_pico_network_config_t settings;
static bool started;
static bool raw_input;
static bool in_cli_command;
#define log_printf(...) ((void)zap_log_printf(settings.log, __VA_ARGS__))


static cli_line_cb_t g_on_line = NULL;
static char ip_addr[16];

// Forward declare the lwIP netif created by cyw43 (provided by pico_cyw43_arch)
static struct netif *get_netif(void) {
    // lwIP sets this to the active interface (your Wi-Fi STA)
    return netif_default;
}


// ======== Simple session state ========
typedef enum {
    CLI_TELNET_DATA = 0,
    CLI_TELNET_IAC,
    CLI_TELNET_OPTION,
    CLI_TELNET_SUBNEGOTIATION,
    CLI_TELNET_SUBNEGOTIATION_IAC,
} cli_telnet_state_t;

typedef struct {
    struct tcp_pcb *pcb;
    char line[RX_LINE_MAX];
    size_t line_len;
    bool suppress_lf;
    cli_telnet_state_t telnet_state;
    uint8_t telnet_command;
    char tx[CLI_TX_BUFFER_MAX];
    size_t tx_start;
    size_t tx_end;
    bool tx_last_was_cr;
    bool tx_overflow;
} cli_session_t;

static cli_session_t g_sess = {0};

// Telnet protocol bytes. Supporting these makes the console work with both
// raw TCP clients (nc/PuTTY Raw) and Telnet-mode terminal clients.
#define TELNET_SE    240u
#define TELNET_SB    250u
#define TELNET_WILL  251u
#define TELNET_WONT  252u
#define TELNET_DO    253u
#define TELNET_DONT  254u
#define TELNET_IAC   255u

// ======== Helpers ========
static void cli_tx_reset(void) {
    g_sess.tx_start = 0u;
    g_sess.tx_end = 0u;
    g_sess.tx_last_was_cr = false;
    g_sess.tx_overflow = false;
}

static void cli_tx_compact(void) {
    if (g_sess.tx_start == 0u) {
        return;
    }
    if (g_sess.tx_start >= g_sess.tx_end) {
        g_sess.tx_start = 0u;
        g_sess.tx_end = 0u;
        return;
    }

    size_t pending = g_sess.tx_end - g_sess.tx_start;
    memmove(g_sess.tx, &g_sess.tx[g_sess.tx_start], pending);
    g_sess.tx_start = 0u;
    g_sess.tx_end = pending;
}

static void cli_tx_flush(void) {
    if (!g_sess.pcb) {
        return;
    }

    bool queued = false;
    while (g_sess.tx_start < g_sess.tx_end) {
        u16_t available = tcp_sndbuf(g_sess.pcb);
        if (available == 0u) {
            break;
        }

        size_t pending = g_sess.tx_end - g_sess.tx_start;
        u16_t chunk = pending < (size_t)available ? (u16_t)pending : available;
        err_t err = tcp_write(g_sess.pcb,
                              &g_sess.tx[g_sess.tx_start],
                              chunk,
                              TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            /* Usually ERR_MEM because lwIP's segment queue is full. The
             * tcp_sent callback will retry once queued bytes are ACKed. */
            break;
        }

        g_sess.tx_start += chunk;
        queued = true;
    }

    if (g_sess.tx_start == g_sess.tx_end) {
        g_sess.tx_start = 0u;
        g_sess.tx_end = 0u;
    }
    if (queued) {
        (void)tcp_output(g_sess.pcb);
    }
}

static bool cli_tx_append_byte(char c) {
    if (g_sess.tx_end >= sizeof(g_sess.tx)) {
        cli_tx_flush();
        cli_tx_compact();
    }
    if (g_sess.tx_end >= sizeof(g_sess.tx)) {
        g_sess.tx_overflow = true;
        return false;
    }

    g_sess.tx[g_sess.tx_end++] = c;
    return true;
}

void zap_pico_network_write(const char *s) {
    if (!g_sess.pcb || !s || !*s) {
        return;
    }

    /* Normalize all CLI text centrally. Existing CRLF remains unchanged;
     * any accidental bare LF becomes CRLF. This keeps PuTTY, Telnet and raw
     * TCP clients consistent without requiring every command to get it right. */
    while (*s) {
        char c = *s++;
        if (c == '\n' && !g_sess.tx_last_was_cr) {
            if (!cli_tx_append_byte('\r')) {
                break;
            }
        }
        if (!cli_tx_append_byte(c)) {
            break;
        }
        g_sess.tx_last_was_cr = (c == '\r');
    }

    cli_tx_flush();
}

bool zap_pico_network_set_raw(bool enabled) {
    if (enabled && (!in_cli_command || !g_sess.pcb || !settings.key)) return false;
    raw_input = enabled;
    return true;
}
bool zap_pico_network_output_idle(void) {
    return g_sess.pcb && g_sess.tx_start == g_sess.tx_end &&
           !g_sess.pcb->unsent && !g_sess.pcb->unacked;
}
static void cli_prompt(void) {
    if (!raw_input) zap_pico_network_write(settings.prompt);
}
static void cli_banner(void) {
    zap_pico_network_write(settings.banner);
    cli_prompt();
}

static void cli_session_reset_input(void) {
    if (raw_input && settings.screen_closed) settings.screen_closed();
    raw_input = false;
    g_sess.line_len = 0u;
    g_sess.line[0] = '\0';
    g_sess.suppress_lf = false;
    g_sess.telnet_state = CLI_TELNET_DATA;
    g_sess.telnet_command = 0u;
}

static err_t on_cli_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    (void)arg;
    (void)tpcb;
    (void)len;
    cli_tx_flush();
    return ERR_OK;
}

static void cli_send_telnet_reject(uint8_t command, uint8_t option) {
    if (!g_sess.pcb) {
        return;
    }

    // DO and WILL are requests that require rejection. DONT and WONT are
    // already negative acknowledgements and must not be answered repeatedly.
    uint8_t response[3] = { TELNET_IAC, 0u, option };
    if (command == TELNET_DO) {
        response[1] = TELNET_WONT;
    } else if (command == TELNET_WILL) {
        response[1] = TELNET_DONT;
    } else {
        return;
    }

    err_t err = tcp_write(g_sess.pcb,
                          response,
                          (u16_t)sizeof(response),
                          TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        (void)tcp_output(g_sess.pcb);
    }
}

static void cli_execute_line(void) {
    /* Clients send characters immediately with local echo disabled. */
    zap_pico_network_write("\r\n");
    g_sess.line[g_sess.line_len] = '\0';

    if (g_sess.line_len > 0u && g_on_line) {
        char cli_out[256];
        cli_out[0] = '\0';
        in_cli_command = true;
        (void)g_on_line(g_sess.line, cli_out, sizeof(cli_out));
        in_cli_command = false;
    }

    g_sess.line_len = 0u;
    g_sess.line[0] = '\0';
    cli_prompt();
}

static void cli_append_data_byte(uint8_t byte) {
    char c = (char)byte;
    if (raw_input) {
        /* Only suppress framing after the CR that entered raw mode. */
        if (g_sess.suppress_lf && (byte == '\n' || byte == 0u)) {
            g_sess.suppress_lf = false;
            return;
        }
        g_sess.suppress_lf = false;
        settings.key(byte);
        return;
    }

    // Telnet NVT may encode a bare CR as CR NUL. The NUL is framing, not data.
    if (byte == 0u) {
        return;
    }

    // Accept CR, LF, CRLF, and CR-NUL line endings. Previously only LF was a
    // terminator, which made behaviour dependent on the terminal program.
    if (c == '\r') {
        cli_execute_line();
        g_sess.suppress_lf = true;
        return;
    }
    if (c == '\n') {
        if (g_sess.suppress_lf) {
            g_sess.suppress_lf = false;
            return;
        }
        cli_execute_line();
        return;
    }
    g_sess.suppress_lf = false;

    // Basic backspace/delete editing.
    if (byte == 0x08u || byte == 0x7fu) {
        if (g_sess.line_len > 0u) {
            g_sess.line_len--;
            g_sess.line[g_sess.line_len] = '\0';
            zap_pico_network_write("\b \b");
        }
        return;
    }

    // Ignore other ASCII control bytes rather than allowing them to poison
    // the first command received from a terminal client.
    if ((unsigned char)c < 0x20u) {
        return;
    }

    if (g_sess.line_len < (RX_LINE_MAX - 1u)) {
        g_sess.line[g_sess.line_len++] = c;
        g_sess.line[g_sess.line_len] = '\0';
        const char echo[] = { c, '\0' };
        zap_pico_network_write(echo);
    }
}

static void cli_process_received_byte(uint8_t byte) {
    switch (g_sess.telnet_state) {
        case CLI_TELNET_DATA:
            if (byte == TELNET_IAC) {
                g_sess.telnet_state = CLI_TELNET_IAC;
            } else {
                cli_append_data_byte(byte);
            }
            break;

        case CLI_TELNET_IAC:
            if (byte == TELNET_IAC) {
                if (raw_input) settings.key(byte);
                // Escaped literal 0xFF is not valid command text; consume it
                // without adding it to the line so parsing remains synchronized.
                g_sess.telnet_state = CLI_TELNET_DATA;
            } else if (byte == TELNET_DO || byte == TELNET_DONT ||
                       byte == TELNET_WILL || byte == TELNET_WONT) {
                g_sess.telnet_command = byte;
                g_sess.telnet_state = CLI_TELNET_OPTION;
            } else if (byte == TELNET_SB) {
                g_sess.telnet_state = CLI_TELNET_SUBNEGOTIATION;
            } else {
                g_sess.telnet_state = CLI_TELNET_DATA;
            }
            break;

        case CLI_TELNET_OPTION:
            cli_send_telnet_reject(g_sess.telnet_command, byte);
            g_sess.telnet_command = 0u;
            g_sess.telnet_state = CLI_TELNET_DATA;
            break;

        case CLI_TELNET_SUBNEGOTIATION:
            if (byte == TELNET_IAC) {
                g_sess.telnet_state = CLI_TELNET_SUBNEGOTIATION_IAC;
            }
            break;

        case CLI_TELNET_SUBNEGOTIATION_IAC:
            g_sess.telnet_state = (byte == TELNET_SE)
                                      ? CLI_TELNET_DATA
                                      : CLI_TELNET_SUBNEGOTIATION;
            break;

        default:
            g_sess.telnet_state = CLI_TELNET_DATA;
            break;
    }
}

bool zap_pico_network_status(char *buf, size_t capacity) {
    if (!buf || !capacity) {
        return false;
    }

    struct netif *netif = get_netif();
    if (!netif) {
        return false;
    }

    const ip4_addr_t *ip = netif_ip4_addr(netif);
    const ip4_addr_t *gw = netif_ip4_gw(netif);
    const ip4_addr_t *mask = netif_ip4_netmask(netif);
    char ip_text[16];
    char gw_text[16];
    char mask_text[16];

    if (!ip4addr_ntoa_r(ip, ip_text, sizeof(ip_text)) ||
        !ip4addr_ntoa_r(gw, gw_text, sizeof(gw_text)) ||
        !ip4addr_ntoa_r(mask, mask_text, sizeof(mask_text))) {
        return false;
    }

    snprintf(buf, capacity,
             "WiFi SSID: %s\r\n"
             "IP Address: %s\r\n"
             "Gateway: %s\r\n"
             "Netmask: %s\r\n",
             settings.ssid,
             ip_text,
             gw_text,
             mask_text);

    return true;
}

// ======== TCP callbacks ========
static err_t on_cli_poll(void *arg, struct tcp_pcb *pcb) {
    (void)arg; (void)pcb;
    cli_tx_flush();
    if (raw_input && settings.screen_tick) settings.screen_tick();
    return ERR_OK;
}
static void on_cli_error(void *arg, err_t err) {
    (void)arg;
    (void)err;
    g_sess.pcb = NULL;
    cli_session_reset_input();
    cli_tx_reset();
}

static err_t on_cli_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;
    if (!p) {
        // Connection closed by remote.
        g_sess.pcb = NULL;
        cli_session_reset_input();
        cli_tx_reset();
        tcp_arg(tpcb, NULL);
        tcp_recv(tpcb, NULL);
        tcp_sent(tpcb, NULL);
        tcp_poll(tpcb, NULL, 0);
        tcp_err(tpcb, NULL);
        return tcp_close(tpcb);
    }
    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    // Read every byte in every pbuf before returning the receive window.
    for (struct pbuf *q = p; q != NULL; q = q->next) {
        const uint8_t *data = (const uint8_t *)q->payload;
        for (u16_t i = 0; i < q->len; i++) {
            cli_process_received_byte(data[i]);
        }
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t on_cli_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    (void)arg;
    if (err != ERR_OK || !newpcb) return ERR_VAL;

    // Only allow one session at a time.
    if (g_sess.pcb) {
        tcp_abort(newpcb);
        return ERR_ABRT;
    }

    g_sess.pcb = newpcb;
    cli_session_reset_input();
    cli_tx_reset();

    tcp_arg(newpcb, &g_sess);
    tcp_recv(newpcb, on_cli_recv);
    tcp_sent(newpcb, on_cli_sent);
    tcp_poll(newpcb, on_cli_poll, 1);
    tcp_err(newpcb, on_cli_error);
    tcp_nagle_disable(newpcb); // snappier CLI

    cli_banner();
    return ERR_OK;
}

static struct tcp_pcb *start_cli_server(void) {
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) return NULL;

    if (tcp_bind(pcb, IP_ANY_TYPE, settings.cli_port) != ERR_OK) {
        tcp_close(pcb);
        return NULL;
    }

    pcb = tcp_listen_with_backlog(pcb, 1);
    if (!pcb) {
        return NULL;
    }
    tcp_accept(pcb, on_cli_accept);
    return pcb;
}

// ======== WiFi connect ========
static bool wifi_connect_blocking(void) {
    // STA mode
    cyw43_arch_enable_sta_mode();

    log_printf("Connecting to WiFi SSID '%s'...\n", settings.ssid);

    // WPA2 PSK. For open networks pass NULL password and CYW43_AUTH_OPEN.
    int err = cyw43_arch_wifi_connect_timeout_ms(
        settings.ssid,
        settings.password,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );

    if (err) {
        log_printf("WiFi connect failed: %d\n", err);
        return false;
    }

    const ip4_addr_t *ip = netif_ip4_addr(get_netif());
    log_printf("WiFi connected. IP = %s\n", ip4addr_ntoa(ip));
    ip4addr_ntoa_r(ip, ip_addr, sizeof(ip_addr));
    return true;
}

int zap_pico_network_start(const zap_pico_network_config_t *config) {
    if (!config || started || !config->ssid || !config->password || !config->command ||
        !config->cli_port || !config->banner || !config->prompt) return -1;
    settings=*config;
    cli_line_cb_t on_line=settings.command;

    g_on_line = on_line;


    sleep_ms(1500);
    log_printf("Pico 2W WiFi CLI starting...\n");

    if (cyw43_arch_init()) {
        log_printf("cyw43_arch_init() failed\n");
        return 1;
    }

    bool ok = wifi_connect_blocking();
    if (!ok) {
        log_printf("Retrying in 5s...\n");
        sleep_ms(5000);
        ok = wifi_connect_blocking();
        if (!ok) {
            log_printf("Giving up.\n");
            cyw43_arch_deinit();
            return 1;
        }
    }

    struct tcp_pcb *server = start_cli_server();
    if (!server) {
        log_printf("Failed to start TCP CLI server\n");
        cyw43_arch_deinit();
        return 1;
    }

    log_printf("Connect from your PC:  nc %s %d\n", ip_addr, settings.cli_port);


    if (settings.http_port && !zap_pico_http_start(on_line, settings.http_port, &settings.assets)) {
        log_printf("HTTP server failed to start\n");
    } else if (settings.http_port) {
        log_printf("HTTP server started on port %u\n", (unsigned)settings.http_port);
    }


    started = true;
    // Servers remain active for the life of the firmware.
    // tcp_close(server);
    // cyw43_arch_deinit();
    return 0;
}
