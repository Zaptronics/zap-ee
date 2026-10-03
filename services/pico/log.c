#include "zaptronics/platform/pico/log.h"
#include <stdio.h>
typedef struct { char text[ZAP_LOG_LINE_MAX]; } line_t;
static bool submit(void *context, const char *text)
{
    zap_pico_log_t *log=context;
    line_t line;
    snprintf(line.text,sizeof(line.text),"%s",text);
    if (queue_try_add(&log->queue,&line)) return true;
    __atomic_fetch_add(&log->dropped,1u,__ATOMIC_RELAXED);
    return false;
}
void zap_pico_log_init(zap_pico_log_t *log, unsigned depth,
                       void (*write)(void *, const char *), void *context)
{
    queue_init(&log->queue,sizeof(line_t),depth);
    log->dropped=0;
    log->write=write;
    log->context=context;
    log->log=(zap_log_t){ .context=log, .submit=submit };
}
bool zap_pico_log_poll(zap_pico_log_t *log)
{
    line_t line;
    if (!log || !queue_try_remove(&log->queue,&line)) return false;
    if (log->write) log->write(log->context,line.text);
    return true;
}
unsigned zap_pico_log_dropped(const zap_pico_log_t *log)
{ return log ? __atomic_load_n(&log->dropped,__ATOMIC_RELAXED) : 0u; }
