#include <assert.h>
#include <string.h>
#include "zaptronics/platform/framed_spi.h"

typedef struct { int active; int begins; int frames; int ends; uint8_t last[8]; size_t len; } fake_t;
static int begin(void *c, uint32_t t){(void)t;fake_t*f=c;if(f->active)return ZAP_SPI_E_STATE;f->active=1;f->begins++;return 0;}
static int frame(void*c,const uint8_t*tx,uint8_t*rx,size_t n,uint8_t fill,uint32_t t){(void)t;fake_t*f=c;if(!f->active)return ZAP_SPI_E_STATE;f->frames++;f->len=n;for(size_t i=0;i<n;i++){uint8_t v=tx?tx[i]:fill;f->last[i]=v;if(rx)rx[i]=(uint8_t)(v^0xffu);}return 0;}
static int end(void*c){fake_t*f=c;if(!f->active)return ZAP_SPI_E_STATE;f->active=0;f->ends++;return 0;}
int main(void){fake_t f={0};const zap_spi_framed_device_ops_t ops={begin,frame,end};zap_spi_framed_device_t d={&f,&ops,123,0x5a};assert(zap_spi_framed_device_valid(&d));assert(zap_spi_framed_sequence_begin(&d)==0);uint8_t tx[]={1,2,3},rx[3];assert(zap_spi_framed_frame(&d,tx,rx,3)==0);assert(rx[0]==0xfe&&rx[2]==0xfc);assert(zap_spi_framed_frame(&d,NULL,rx,3)==0);assert(f.last[0]==0x5a);assert(zap_spi_framed_sequence_end(&d)==0);assert(f.begins==1&&f.frames==2&&f.ends==1);return 0;}
