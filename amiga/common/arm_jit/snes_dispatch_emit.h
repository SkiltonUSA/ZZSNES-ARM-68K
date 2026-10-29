/* Runtime-bound A32 opcode handlers. No guest address lookup or literal pool.
 * r0-r3/r12 are scratch; all AAPCS preserved registers and SP remain intact. */
#pragma once
#include "register_op.h"
struct cj_snes_layout {
 uint32_t registers,flags,cpu,overclock,one_cycle;
 unsigned cycles,wait,zero,negative;
};
static inline unsigned cj_mov32(uint32_t *p,unsigned reg,uint32_t value){
 p[0]=0xe3000000u|(reg<<12)|((value&0xf000u)<<4)|(value&0xfffu);
 p[1]=0xe3400000u|(reg<<12)|((value>>12)&0xf0000u)|((value>>16)&0xfffu);
 return 2;
}
/* Caller provides 64 words. Timing reads the current overclock controls, rather
 * than baking in ONE_CYCLE at startup; settings/state changes stay effective. */
static inline unsigned cj_emit_snes_dispatch(uint32_t *p,const struct cj_snes_layout *l,
 unsigned reg,int delta,unsigned bits,unsigned timing,unsigned shutdown){
 unsigned n=0;
 if(timing){
  n+=cj_mov32(p+n,12,l->overclock);
  p[n++]=0xe5dc2000u; /* LDRB r2,[r12] */
  p[n++]=0xe3520000u; /* CMP r2,#0 */
  n+=cj_mov32(p+n,12,l->one_cycle);
  p[n++]=0x159c3000u; /* LDRNE r3,[r12] */
  p[n++]=0x03a03006u; /* MOVEQ r3,#6 */
 }
 if(timing||shutdown)n+=cj_mov32(p+n,12,l->cpu);
 if(timing){
  p[n++]=0xe59c2000u|l->cycles;
  p[n++]=0xe0822003u; /* ADD r2,r2,r3 */
  p[n++]=0xe58c2000u|l->cycles;
 }
 if(shutdown){p[n++]=0xe3a02000u;p[n++]=0xe58c2000u|l->wait;}
 n+=cj_mov32(p+n,0,l->registers);
 n+=cj_mov32(p+n,1,l->flags);
 n+=cj_emit(p+n,reg,reg,delta,bits,2,0,l->zero,l->negative);
 return n;
}
