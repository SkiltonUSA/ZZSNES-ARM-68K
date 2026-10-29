/* GPL-3.0-or-later. Included after the Snes9x CPU definitions. */
#include <stddef.h>
#include "cache.h"
int console_jit_init(uintptr_t base){return cj_init(base);}
static int console_jit_op(unsigned op){
 unsigned reg;int delta;
 switch(op){
 case 0xe8:reg=offsetof(SRegisters,X);delta=1;break;
 case 0xc8:reg=offsetof(SRegisters,Y);delta=1;break;
 case 0xca:reg=offsetof(SRegisters,X);delta=-1;break;
 case 0x88:reg=offsetof(SRegisters,Y);delta=-1;break;
 default:return 0;
 }
 unsigned bits=CheckIndex()?8:16;
#ifdef VAR_CYCLES
 CPU.Cycles+=ONE_CYCLE;
#endif
#ifdef CPU_SHUTDOWN
 CPU.WaitAddress=0;
#endif
 cj_execute((uint32_t)(uintptr_t)(CPU.PC-1),op|(bits==16?256:0),&Registers,&ICPU,
   reg,reg,delta,bits,2,0,offsetof(SICPU,_Zero),offsetof(SICPU,_Negative));
 return 1;
}
