/* GPL-3.0-or-later. Experimental A32 register-only instruction emitter.
 * r0: guest registers, r1: separate cached flags (SNES). No guest memory,
 * PC, cycle, interrupt or scheduler changes; those stay with the core.
 * Caller supplies byte offsets <256 and at least 32 instruction words. */
#pragma once
#include <stdint.h>
static inline unsigned cj_emit(uint32_t *p,unsigned src,unsigned dst,int delta,
 unsigned bits,unsigned flags,unsigned packed,unsigned zero,unsigned negative){
 unsigned n=0;
 p[n++]=bits==8?0xe5d02000u|src:0xe1d020b0u|((src&0xf0u)<<4)|(src&15u);
 if(delta)p[n++]=(delta>0?0xe2822001u:0xe2422001u); /* ADD/SUB */
 p[n++]=0xe7e02052u|((bits-1)<<16); /* UBFX r2,r2,#0,#bits */
 p[n++]=bits==8?0xe5c02000u|dst:0xe1c020b0u|((dst&0xf0u)<<4)|(dst&15u);
 if(flags){
  p[n++]=0xe3520000u; /* CMP r2,#0 */
  if(flags==1){
   p[n++]=0xe5d03000u|packed;
   p[n++]=0xe3c33082u;
   p[n++]=0x03833002u; /* ORREQ r3,r3,#Z */
   p[n++]=0xe1a0c022u|((bits-8)<<7); /* MOV r12,r2,LSR #(bits-8) */
   if(bits==8)p[n-1]=0xe1a0c002u; /* zero-shift MOV, not LSR #32 */
   p[n++]=0xe20cc080u;
   p[n++]=0xe183300cu;
   p[n++]=0xe5c03000u|packed;
  }else{
   if(bits==8)p[n++]=0xe1a03002u; /* preserve core cached-byte representation */
   else{p[n++]=0x03a03000u;p[n++]=0x13a03001u;}
   p[n++]=0xe5c13000u|zero;
   p[n++]=bits==8?0xe1a03002u:0xe1a03422u;
   p[n++]=0xe5c13000u|negative;
  }
 }
 p[n++]=0xe12fff1eu;
 return n;
}
