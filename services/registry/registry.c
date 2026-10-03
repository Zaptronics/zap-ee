#include "zaptronics/services/registry.h"
#include <string.h>
static bool valid(const zap_registry_t *r)
{ return r && (r->count == 0 || (r->entries && r->initialized)); }
static void event(zap_registry_t *r, size_t i, const char *op, int rc)
{ if (r->event) r->event(r->context, &r->entries[i], op, rc); }
static size_t index_of(zap_registry_t *r, const char *name)
{
    if (!valid(r) || !name) return SIZE_MAX;
    for (size_t i=0; i<r->count; ++i)
        if (r->entries[i].short_name && strcmp(r->entries[i].short_name,name)==0) return i;
    return SIZE_MAX;
}
int zap_registry_deinitialize(zap_registry_t *r, const char *name)
{
    size_t i=index_of(r,name);
    if (i==SIZE_MAX) return -1;
    int rc=r->entries[i].deinit ? r->entries[i].deinit() : 0;
    r->initialized[i]=false;
    event(r,i,"deinit",rc);
    return rc;
}
int zap_registry_reinitialize(zap_registry_t *r, const char *name)
{
    size_t i=index_of(r,name);
    if (i==SIZE_MAX) return -1;
    module_entry_t *m=&r->entries[i];
    if (m->deinit) {
        int rc=m->deinit();
        if (rc) { r->initialized[i]=false; event(r,i,"deinit",rc); return rc; }
    }
    r->initialized[i]=false;
    if (!m->init) return -2;
    int rc=m->init(m->mod_inst,m->bus_ctx,m->addr,m->en_pin);
    r->initialized[i]=rc==0;
    event(r,i,"init",rc);
    return rc;
}
int zap_registry_init_all(zap_registry_t *r, bool rollback)
{
    if (!valid(r)) return -1;
    int first=0;
    for (size_t i=0; i<r->count; ++i) {
        module_entry_t *m=&r->entries[i];
        if (r->initialized[i] || !m->init) continue;
        int rc=m->init(m->mod_inst,m->bus_ctx,m->addr,m->en_pin);
        r->initialized[i]=rc==0;
        event(r,i,"init",rc);
        if (rc && !first) first=rc;
        if (rc && rollback) { (void)zap_registry_deinit_all(r); return rc; }
    }
    return first;
}
int zap_registry_poll_all(zap_registry_t *r)
{
    if (!valid(r)) return -1;
    int errors=0;
    for (size_t i=0; i<r->count; ++i) {
        if (!r->initialized[i] || !r->entries[i].poll) continue;
        int rc=r->entries[i].poll();
        if (rc) { ++errors; event(r,i,"poll",rc); }
        if (r->yield) r->yield(r->context);
    }
    return errors;
}
int zap_registry_deinit_all(zap_registry_t *r)
{
    if (!valid(r)) return -1;
    int errors=0;
    for (size_t i=r->count; i-- > 0;) {
        if (!r->initialized[i]) continue;
        int rc=r->entries[i].deinit ? r->entries[i].deinit() : 0;
        if (rc) ++errors;
        r->initialized[i]=false;
        event(r,i,"deinit",rc);
    }
    return errors;
}
module_entry_t *zap_registry_find(zap_registry_t *r, const char *name)
{
    size_t i=index_of(r,name);
    return i != SIZE_MAX && r->initialized[i] ? &r->entries[i] : NULL;
}
const module_entry_t *zap_registry_get(const zap_registry_t *r,size_t i)
{ return valid(r) && i<r->count ? &r->entries[i] : NULL; }
bool zap_registry_is_initialized(const zap_registry_t *r,size_t i)
{ return valid(r) && i<r->count && r->initialized[i]; }
