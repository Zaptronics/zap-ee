#include <assert.h>
#include <string.h>
#include "zaptronics/modules/ztm_stepper.h"
int main(void){uint8_t f[4];assert(ztm_stepper_compose_frame(1,0x11,f));assert(f[0]==0&&f[1]==0&&f[2]==0&&f[3]==0x11);assert(ztm_stepper_compose_frame(4,0x44,f));assert(f[0]==0x44&&f[1]==0&&f[2]==0&&f[3]==0);assert(!ztm_stepper_compose_frame(0,1,f));return 0;}
