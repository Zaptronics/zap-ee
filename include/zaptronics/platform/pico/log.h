#pragma once
#include "pico/util/queue.h"
#include "zaptronics/services/log.h"
typedef struct {
    queue_t queue;
    zap_log_t log;
    unsigned dropped;
    void *context;
    void (*write)(void *context, const char *line);
} zap_pico_log_t;
/* Initialize once before producers run. Sink runs only from poll(). */
void zap_pico_log_init(zap_pico_log_t *log, unsigned depth,
                       void (*write)(void *, const char *), void *context);
bool zap_pico_log_poll(zap_pico_log_t *log);
unsigned zap_pico_log_dropped(const zap_pico_log_t *log);
