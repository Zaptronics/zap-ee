#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*module_init_fn_t)(void *mod_inst, void *bus_ctx, int8_t addr, int8_t en_pin);
typedef int (*module_poll_fn_t)(void);
typedef int (*module_deinit_fn_t)(void);

typedef struct module_entry {
    const char         *short_name;
    const char         *long_name;
    module_init_fn_t    init;
    void               *bus_ctx;
    void               *mod_inst;
    module_poll_fn_t    poll;
    module_deinit_fn_t  deinit;
    int8_t              addr;
    int8_t              en_pin;
} module_entry_t;


/* Caller owns entries, initialized flags, and optional event/yield callbacks.
 * Serialize registry mutations in the application's execution context. */
typedef struct {
    module_entry_t *entries;
    bool *initialized;
    size_t count;
    void *context;
    void (*event)(void *context, const module_entry_t *entry, const char *operation, int result);
    void (*yield)(void *context);
} zap_registry_t;
int zap_registry_init_all(zap_registry_t *registry, bool rollback_on_fail);
int zap_registry_poll_all(zap_registry_t *registry);
int zap_registry_deinit_all(zap_registry_t *registry);
module_entry_t *zap_registry_find(zap_registry_t *registry, const char *name);
int zap_registry_reinitialize(zap_registry_t *registry, const char *name);
int zap_registry_deinitialize(zap_registry_t *registry, const char *name);
const module_entry_t *zap_registry_get(const zap_registry_t *registry, size_t index);
bool zap_registry_is_initialized(const zap_registry_t *registry, size_t index);

#ifdef __cplusplus
}
#endif
