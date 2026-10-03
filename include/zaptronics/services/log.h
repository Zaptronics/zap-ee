#pragma once
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
#define ZAP_LOG_LINE_MAX 512u
typedef struct {
    void *context;
    bool (*submit)(void *context, const char *line);
} zap_log_t;
bool zap_log_vprintf(const zap_log_t *log, const char *format, va_list args);
bool zap_log_printf(const zap_log_t *log, const char *format, ...);

#ifdef __cplusplus
}
#endif
