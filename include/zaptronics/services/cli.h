#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef int (*cli_cmd_fn_t)(int argc, char **argv, char *out, size_t out_len);

typedef struct {
    const char   *name;
    const char   *help;
    cli_cmd_fn_t  fn;
} cli_cmd_t;

typedef struct {
    const char    *name;      // group name, e.g. "pump"
    const char    *help;      // group help
    const cli_cmd_t *cmds;    // subcommands
    size_t         cmd_count;
} cli_group_t;

bool zap_cli_parse_u32(const char *s, uint32_t *out);
bool zap_cli_parse_float(const char *s, float *out);
int zap_cli_tokenize(char *line, char **argv, int argv_max);
const cli_cmd_t *zap_cli_find_cmd(const cli_cmd_t *cmds, size_t n, const char *name);
const cli_group_t *zap_cli_find_group(const cli_group_t *groups, size_t n, const char *name);

#ifdef __cplusplus
}
#endif
