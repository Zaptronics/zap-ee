#include <assert.h>
#include "zaptronics/drivers/tla2518.h"
typedef struct { unsigned begins,frames,ends; bool active,fail; } fake_t;
static int begin(void *ctx,uint32_t timeout)
{ (void)timeout; fake_t *f=ctx; assert(!f->active); f->active=true; f->begins++; return 0; }
static int frame(void *ctx,const uint8_t *tx,uint8_t *rx,size_t n,uint8_t fill,uint32_t timeout)
{
    (void)fill; (void)timeout;
    fake_t *f=ctx;
    assert(f->active && n==3u);
    f->frames++;
    if (f->fail) return ZAP_SPI_E_IO;
    if (rx) { assert(tx[0]==0u); rx[0]=0x5Au; }
    else assert(tx[0]==0x10u);
    return 0;
}
static int end(void *ctx) { fake_t *f=ctx; assert(f->active); f->active=false; f->ends++; return 0; }
int main(void)
{
    fake_t fake={0};
    const zap_spi_framed_device_ops_t ops={begin,frame,end};
    const zap_spi_framed_device_t spi={.context=&fake,.ops=&ops};
    tla2518_dev_t dev={.spi=&spi,.vref_mv=3300u};
    uint8_t value=0;
    assert(tla2518_reg_read(&dev,0u,&value)==0 && value==0x5Au);
    assert(fake.begins==1u && fake.frames==2u && fake.ends==1u);
    fake.fail=true;
    assert(tla2518_reg_read(&dev,0u,&value)==TLA2518_ERR_SPI);
    assert(fake.ends==2u && !fake.active);
    assert(tla2518_code_to_uV(&dev,4095u)==3300000u);
    return 0;
}
