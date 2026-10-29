/* GPL-3.0-or-later. Experimental single-Core1 register translation cache.
 * The firmware must have granted this application exclusive Core1 ownership.
 * Allocation and executable page attributes are supplied by the host/MMU plan.
 * Every dispatch revalidates opcode/mode; no guest memory access is bypassed. */
#pragma once
#include <stdint.h>
#include "register_op.h"
#define CJ_BYTES (8u*1024u*1024u)
#define CJ_SLOTS (CJ_BYTES/128u)
struct cj_tag {uint32_t key;uint16_t signature,valid;};
static struct cj_tag cj_tags[CJ_SLOTS];
static uint32_t *cj_code;
static unsigned cj_hits,cj_compiles;
static inline void cj_commit(uint32_t *code,unsigned words){
 uintptr_t begin=(uintptr_t)code&~31u,end=(uintptr_t)(code+words);unsigned zero=0;
 for(uintptr_t p=begin;p<end;p+=32)__asm__ volatile("mcr p15,0,%0,c7,c11,1"::"r"(p):"memory");
 __asm__ volatile("dsb sy":::"memory");
 for(uintptr_t p=begin;p<end;p+=32)__asm__ volatile("mcr p15,0,%0,c7,c5,1"::"r"(p):"memory");
 __asm__ volatile("mcr p15,0,%0,c7,c5,6\n dsb sy\n isb"::"r"(zero):"memory");
}
static inline int cj_init(uintptr_t base){
 cj_code=(uint32_t*)((base+CJ_BEGIN+4095u)&~4095u);
 /* Rewritten-code witness, before any guest execution. */
 for(unsigned n=41;n<44;n++){
  cj_code[0]=0xe3a00000u|n;cj_code[1]=0xe12fff1eu;cj_commit(cj_code,2);
  if(((unsigned(*)(void))cj_code)()!=n){cj_code=0;return 0;}
 }
 return 1;
}
static inline void cj_execute(uint32_t key,unsigned signature,void *registers,void *flags,
 unsigned src,unsigned dst,int delta,unsigned bits,unsigned kind,
 unsigned packed,unsigned zero,unsigned negative){
 unsigned slot=(key^(key>>16))&(CJ_SLOTS-1);
 struct cj_tag *tag=&cj_tags[slot];uint32_t *code=cj_code+slot*32u;
 if(!tag->valid||tag->key!=key||tag->signature!=signature){
  unsigned words=cj_emit(code,src,dst,delta,bits,kind,packed,zero,negative);
  cj_commit(code,words);tag->key=key;tag->signature=signature;tag->valid=1;cj_compiles++;
 }else cj_hits++;
 ((void(*)(void*,void*))code)(registers,flags);
}
