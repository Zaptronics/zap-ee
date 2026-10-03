#include <assert.h>
#include <string.h>
#include "zaptronics/drivers/pcal6416.h"
typedef struct { uint8_t regs[256]; uint8_t order[16]; unsigned count; bool fail; } fake_t;
static int write_bytes(void *ctx, uint8_t address, const uint8_t *data, size_t n, uint32_t timeout)
{
    (void)timeout;
    fake_t *f=ctx;
    assert(address==0x20u && n==2u);
    if (f->fail) return ZAP_I2C_E_IO;
    f->order[f->count++]=data[0]; f->regs[data[0]]=data[1]; return 0;
}
static int read_bytes(void *ctx,uint8_t address,const uint8_t *tx,size_t tn,uint8_t *rx,size_t rn,uint32_t timeout)
{
    (void)address; (void)timeout;
    assert(tn==1u && rn==1u);
    *rx=((fake_t *)ctx)->regs[*tx]; return 0;
}
int main(void)
{
    fake_t fake={0};
    const zap_i2c_bus_ops_t ops={.write=write_bytes,.write_read=read_bytes};
    const zap_i2c_bus_t bus={.context=&fake,.ops=&ops,.timeout_us=5000u};
    pcal6416a_t dev={0};
    assert(pcal6416a_init(&dev,&bus,0x20u,0xA550u,0u,NULL));
    assert(fake.order[1]==PCAL6416A_REG_OUTPUT0 && fake.order[2]==PCAL6416A_REG_OUTPUT1);
    assert(fake.order[3]==PCAL6416A_REG_CONFIG0 && fake.order[4]==PCAL6416A_REG_CONFIG1);
    fake.fail=true;
    assert(!pcal6416a_write_pin(&dev,0u,true));
    assert(dev.out0==0u);
    fake.fail=false;
    assert(pcal6416a_write_pin(&dev,1u,true));
    assert(dev.out0==2u && fake.regs[PCAL6416A_REG_OUTPUT0]==2u);
    assert(!pcal6416a_write_pin(&dev,16u,true));
    return 0;
}
