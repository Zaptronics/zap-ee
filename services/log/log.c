#include "zaptronics/services/log.h"
#include <stdio.h>
#include <string.h>
bool zap_log_vprintf(const zap_log_t *log, const char *format, va_list args)
{
    if (!log || !log->submit || !format) return false;
    char line[ZAP_LOG_LINE_MAX];
    va_list copy;
    va_copy(copy,args);
    int result=vsnprintf(line,sizeof(line)-2u,format,copy);
    va_end(copy);
    if (result<0) return false;
    size_t n=strlen(line);
    if (!n || line[n-1]!='\n') { line[n++]='\n'; line[n]='\0'; }
    return log->submit(log->context,line);
}
bool zap_log_printf(const zap_log_t *log, const char *format, ...)
{
    va_list args;
    va_start(args,format);
    bool ok=zap_log_vprintf(log,format,args);
    va_end(args);
    return ok;
}
