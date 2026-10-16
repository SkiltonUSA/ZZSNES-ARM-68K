// GPL-3.0-or-later. Private cached core; explicit publication to the 68k.
#include "wire.h"
#include "mmu.h"
#include "nes_api.h"
extern "C" unsigned fc_state_store(uint8_t *,unsigned);
extern "C" int fc_state_load(const uint8_t *,unsigned);
extern "C" unsigned ps_video_height(void),ps_frame_us(void);
#ifdef NES_DIRECT
extern "C" uint32_t nes_present(volatile uint8_t *,uint32_t,uint32_t,uint32_t,uint32_t);
#endif
static volatile uint8_t *shared;
static void barrier(){__asm__ volatile("dsb sy":::"memory");}
static int stopped(){return ad_get(shared,ZZ_STOP)!=0;}
static uint64_t ticks(){volatile uint32_t *t=(volatile uint32_t *)0xf8f00200u;uint32_t hi,lo;do{hi=t[1];lo=t[0];}while(hi!=t[1]);return (uint64_t)hi<<32|lo;}
static void clean_range(volatile uint8_t *d,uint32_t n){
 for(uint32_t a=(uint32_t)d&~31u;a<(uint32_t)d+n;a+=32)__asm__ volatile("mcr p15,0,%0,c7,c10,1"::"r"(a):"memory");barrier();
}
static void publish_frame(){
 uint16_t *p=nes_pixels();volatile uint8_t *d=shared+NES_OUTPUT;
 // RGB emphasis remains in the core/hash, but is omitted in this 64-pen preview.
 #ifdef NES_PLAYER
 for(unsigned i=0;i<NES_PIXELS;i++){d[2*i]=p[i]>>8;d[2*i+1]=(uint8_t)p[i];}
 #else
 for(unsigned i=0;i<NES_PIXELS;i++)d[i]=p[i]&63;
 #endif
 // Only our output lines, inner cache maintenance. The map is outer NC;
 // no PL310 registers, global operations, or framebuffer aliases are touched.
 for(uint32_t a=(uint32_t)d;a<(uint32_t)d+NES_OUTPUT_BYTES;a+=32)
  __asm__ volatile("mcr p15,0,%0,c7,c10,1"::"r"(a):"memory");
 barrier();
}
extern "C" void zz_worker(volatile uint8_t *base){
 shared=base;uint32_t sctlr,midr,mpidr,seq=0;bool initialized=false;
 __asm__ volatile("mrc p15,0,%0,c1,c0,0":"=r"(sctlr));
 __asm__ volatile("mrc p15,0,%0,c0,c0,0":"=r"(midr));
 __asm__ volatile("mrc p15,0,%0,c0,c0,5":"=r"(mpidr));
 if((sctlr&0x1005u)!=0x1005u || (mpidr&255)!=1 || !(*(volatile uint32_t *)0xf8f00208u&1))return;
 for(unsigned i=0;i<256;i++)ad_put(base,MB_WITNESS+i*4,ad_get(base,ZZ_CONTROL)^(i*0x01010101u));
 ad_put(base,ZZ_DIAG+4,sctlr);ad_put(base,ZZ_DIAG+8,midr);ad_put(base,ZZ_DIAG+12,mpidr);ad_put(base,ZZ_DIAG,ZZ_READY);barrier();
 while(!stopped()){
  volatile uint8_t *cmd=base+MB_HOST,*reply=base+MB_REPLY;uint32_t next=ad_get(cmd,0);if(next==seq)continue;
  barrier();uint32_t request_start=(uint32_t)ticks();uint32_t op=ad_get(cmd,4),token=ad_get(cmd,8),arg=ad_get(cmd,12),error=0,elapsed=0;
  if(next!=seq+1 || token!=(next^ad_get(base,ZZ_CONTROL)))error=4;
  else if(op==NES_INIT && !initialized){
   error=arg>NES_ROM_LIMIT?1:nes_init((const uint8_t *)base+NES_ROM,arg,(uint16_t *)(base+NES_BUFFER_A),(uint16_t *)(base+NES_BUFFER_B));
   initialized=!error;

#ifdef NES_PLAYER
  }else if((op==NES_SAVE_LOAD||op==NES_SAVE_STORE)&&initialized){
   uint8_t *p;unsigned n=nes_save(&p);if(arg!=n||n>NES_SAVE_LIMIT)error=9;
   else if(op==NES_SAVE_LOAD){
    for(uint32_t a=(uint32_t)(base+NES_SAVE);a<(uint32_t)(base+NES_SAVE)+n;a+=32)__asm__ volatile("mcr p15,0,%0,c7,c6,1"::"r"(a):"memory");barrier();
    for(unsigned i=0;i<n;i++)p[i]=base[NES_SAVE+i];
   }else {for(unsigned i=0;i<n;i++)base[NES_SAVE+i]=p[i];clean_range(base+NES_SAVE,n);}
#endif
  }else if((op==10||op==11)&&initialized){
   uint32_t result=0;
   if(op==10&&arg>=32&&arg<=NES_SAVE_LIMIT){
    for(uint32_t a=(uint32_t)(base+NES_SAVE)&~31u;a<(uint32_t)(base+NES_SAVE)+arg;a+=32)__asm__ volatile("mcr p15,0,%0,c7,c6,1"::"r"(a):"memory");barrier();
    result=fc_state_load((const uint8_t *)base+NES_SAVE,arg);if(result)publish_frame();
   }else if(op==11){result=fc_state_store((uint8_t *)base+NES_SAVE,NES_SAVE_LIMIT);if(result)clean_range(base+NES_SAVE,result);}
   ad_put(reply,52,result);
  }else if(op==NES_FRAME && initialized){
   uint32_t start=(uint32_t)ticks();error=nes_frame((uint16_t)arg,stopped);elapsed=(uint32_t)ticks()-start;
   if(!error && (arg&65536u))publish_frame();
#ifdef NES_PLAYER
   const int16_t *pcm;unsigned count=nes_audio(&pcm);
   if(count>1024)error=8;
   else {for(unsigned i=0;i<count*2;i++){base[NES_AUDIO+2*i]=(uint16_t)pcm[i]>>8;base[NES_AUDIO+2*i+1]=(uint8_t)pcm[i];}clean_range(base+NES_AUDIO,count*4);ad_put(reply,40,count);}
#endif
  }
#ifdef NES_DIRECT
  else if(op==NES_PRESENT && initialized){
   uint32_t start=(uint32_t)ticks();
   error=nes_present(base,arg,ad_get(cmd,16),ad_get(cmd,20),ad_get(cmd,24));
   elapsed=(uint32_t)ticks()-start;
  }
#endif
  else if(op!=NES_CLOCK)error=4;
  uint64_t now=ticks();ad_put(reply,4,error);ad_put(reply,8,(uint32_t)now);ad_put(reply,12,(uint32_t)(now>>32));
  ad_put(reply,56,initialized?ps_video_height():224);ad_put(reply,60,initialized?ps_frame_us():0);
  ad_put(reply,16,initialized?nes_frames():0);ad_put(reply,20,initialized?nes_cycles():0);ad_put(reply,24,initialized?nes_hash():0);ad_put(reply,28,elapsed);
#ifdef NES_PLAYER
  if(initialized){uint8_t *p;ad_put(reply,44,nes_save(&p));}
#endif
  // Transfer checksum independently checks cached output publication.
  uint32_t hash=2166136261u;
  if(op==NES_FRAME && !error && (arg&65536u))for(unsigned i=0;i<NES_OUTPUT_BYTES;i++)hash=(hash^base[NES_OUTPUT+i])*16777619u;
  ad_put(reply,48,(uint32_t)ticks()-request_start);ad_put(reply,32,hash);ad_put(reply,68,token^ZZ_XOR);barrier();ad_put(reply,0,next);barrier();seq=next;
 }
}
