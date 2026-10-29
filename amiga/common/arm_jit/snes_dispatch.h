/* Included in cpuexec.c. The original dispatcher and event checks are intact.
 * Four opcode tables select the eight register handlers by the CPU's X mode. */
#include <stddef.h>
#include "cache.h"
#include "snes_dispatch_emit.h"
int console_jit_init(uintptr_t base){
 if(!cj_init(base))return 0;
 const struct cj_snes_layout layout={
  (uint32_t)(uintptr_t)&Registers,(uint32_t)(uintptr_t)&ICPU,
  (uint32_t)(uintptr_t)&CPU,(uint32_t)(uintptr_t)&overclock_cycles,
  (uint32_t)(uintptr_t)&one_c,offsetof(SCPUState,Cycles),
  offsetof(SCPUState,WaitAddress),offsetof(SICPU,_Zero),offsetof(SICPU,_Negative)};
 const unsigned opcodes[]={0xe8,0xc8,0xca,0x88};
 unsigned timing=0,shutdown=0;
#ifdef VAR_CYCLES
 timing=1;
#endif
#ifdef CPU_SHUTDOWN
 shutdown=1;
#endif
 for(unsigned wide=0;wide<2;wide++)for(unsigned i=0;i<4;i++){
  uint32_t *code=cj_code+(wide*4+i)*64;
  unsigned reg=(i&1)?offsetof(SRegisters,Y):offsetof(SRegisters,X);
  unsigned words=cj_emit_snes_dispatch(code,&layout,reg,i<2?1:-1,wide?16:8,timing,shutdown);
  cj_commit(code,words);
  void (*handler)(void)=(void(*)(void))code;
  (wide?S9xOpcodesM1X0:S9xOpcodesM1X1)[opcodes[i]].S9xOpcode=handler;
  (wide?S9xOpcodesM0X0:S9xOpcodesM0X1)[opcodes[i]].S9xOpcode=handler;
 }
 return 1;
}
