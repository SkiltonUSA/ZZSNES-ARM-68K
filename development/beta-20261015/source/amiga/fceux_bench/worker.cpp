// GPL-3.0-or-later. Private cached core; explicit publication to the 68k.
#include "wire.h"
#include "mmu.h"
#include "core.h"
static void barrier(){__asm__ volatile("dsb sy":::"memory");}
static uint64_t ticks(){volatile uint32_t *t=(volatile uint32_t *)0xf8f00200u;uint32_t hi,lo;do{hi=t[1];lo=t[0];}while(hi!=t[1]);return (uint64_t)hi<<32|lo;}
extern "C" void zz_worker(volatile uint8_t *base){
 uint32_t sctlr,midr,mpidr,seq=0,frames=0;bool initialized=false;
 __asm__ volatile("mrc p15,0,%0,c1,c0,0":"=r"(sctlr));
 __asm__ volatile("mrc p15,0,%0,c0,c0,0":"=r"(midr));
 __asm__ volatile("mrc p15,0,%0,c0,c0,5":"=r"(mpidr));
 if((sctlr&0x1005u)!=0x1005u || (mpidr&255)!=1 || !(*(volatile uint32_t *)0xf8f00208u&1))return;
 for(unsigned i=0;i<256;i++)ad_put(base,MB_WITNESS+i*4,ad_get(base,ZZ_CONTROL)^(i*0x01010101u));
 ad_put(base,ZZ_DIAG+4,sctlr);ad_put(base,ZZ_DIAG+8,midr);ad_put(base,ZZ_DIAG+12,mpidr);ad_put(base,ZZ_DIAG,ZZ_READY);barrier();
 while(!ad_get(base,ZZ_STOP)){
  volatile uint8_t *cmd=base+MB_HOST,*reply=base+MB_REPLY;uint32_t next=ad_get(cmd,0);if(next==seq)continue;
  barrier();uint32_t op=ad_get(cmd,4),token=ad_get(cmd,8),arg=ad_get(cmd,12),error=0,elapsed=0;
  if(next!=seq+1 || token!=(next^ad_get(base,ZZ_CONTROL)))error=4;
  else if(op==FC_INIT && !initialized){error=arg>FC_ROM_LIMIT?1:fc_init((const uint8_t *)base+FC_ROM,arg);initialized=!error;}
  else if(op==FC_FRAME && initialized){uint32_t start=(uint32_t)ticks();error=fc_frame((uint8_t)arg);elapsed=(uint32_t)ticks()-start;if(!error)frames++;}
  else if(op!=FC_CLOCK)error=4;
  uint64_t now=ticks();ad_put(reply,4,error);ad_put(reply,8,(uint32_t)now);ad_put(reply,12,(uint32_t)(now>>32));
  ad_put(reply,16,frames);ad_put(reply,20,fc_cycles());ad_put(reply,24,initialized?fc_video_hash():0);ad_put(reply,28,elapsed);
  ad_put(reply,32,initialized?fc_audio_hash():0);ad_put(reply,36,fc_samples());ad_put(reply,40,fc_nonzero());
  ad_put(reply,68,token^ZZ_XOR);barrier();ad_put(reply,0,next);barrier();seq=next;
 }
}
