#include <assert.h>
#include "zaptronics/services/registry.h"
static unsigned stops;
static int start(void *instance, void *bus, int8_t addr, int8_t en)
{ (void)bus; (void)addr; (void)en; return *(int *)instance; }
static int stop(void) { ++stops; return 0; }
int main(void)
{
    int healthy=0,failed=-7;
    module_entry_t entries[]={
        {.short_name="good",.init=start,.deinit=stop,.mod_inst=&healthy},
        {.short_name="bad",.init=start,.deinit=stop,.mod_inst=&failed}
    };
    bool flags[2]={false,false};
    zap_registry_t r={.entries=entries,.initialized=flags,.count=2u};
    assert(zap_registry_init_all(&r,false)==-7);
    assert(zap_registry_find(&r,"good") && !zap_registry_find(&r,"bad"));
    assert(zap_registry_deinit_all(&r)==0 && stops==1u);
    assert(zap_registry_init_all(&r,true)==-7);
    assert(!flags[0] && !flags[1] && stops==2u);
    return 0;
}
