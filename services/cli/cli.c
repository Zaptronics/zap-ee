#include "zaptronics/services/cli.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
/* ---- Argument parsing ---- */
bool zap_cli_parse_u32(const char *s, uint32_t *out) {
    if (!s || !*s || !out || *s == '-') return false;
    errno = 0;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 0); /* decimal or 0x-prefixed hex */
    if (errno != 0 || !end || *end != '\0' || v > UINT32_MAX) return false;
    *out = (uint32_t)v;
    return true;
}

bool zap_cli_parse_float(const char *s, float *out) {
    if (!s || !*s || !out) return false;
    errno = 0;
    char *end = NULL;
    float v = strtof(s, &end);
    if (errno != 0 || !end || *end != '\0' || !isfinite(v)) return false;
    *out = v;
    return true;
}



int zap_cli_tokenize(char *line, char **argv, int argv_max) {
    if (!line || !argv || argv_max <= 0) return 0;
    int argc = 0;
    char *p = line;

    while (*p) {
        // skip whitespace
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        if (argc >= argv_max) break;

        argv[argc++] = p;

        // advance to end of token
        while (*p && !isspace((unsigned char)*p)) p++;

        // null-terminate token
        if (*p) *p++ = '\0';
    }
    return argc;
}

/* ---- Helpers ---- */
const cli_cmd_t *zap_cli_find_cmd(const cli_cmd_t *cmds, size_t n, const char *name) {
    if (!cmds || !name) return NULL;
    for (size_t i = 0; i < n; i++) {
        if (strcmp(cmds[i].name, name) == 0) return &cmds[i];
    }
    return NULL;
}

const cli_group_t *zap_cli_find_group(const cli_group_t *groups, size_t n, const char *name) {
    if (!groups || !name) return NULL;
    for (size_t i = 0; i < n; i++) {
        if (strcmp(groups[i].name, name) == 0) return &groups[i];
    }
    return NULL;
}





