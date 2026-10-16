// GPL-3.0-or-later. Bounded private-frame presentation; map only the owned slot.
#include "wire.h"
#include "mmu.h"
#include "direct_plan.h"
#include "player_palette.h"
#ifdef EM_SNES
#include "view_fast.h"
#else
#include "view.h"
#endif
#include "nes_api.h"
#include "row_copy_arm.h"
#include "display_math.h"
extern "C" unsigned db_neon_available(uint32_t*);
extern "C" void db_neon_snapshot(uint32_t*),db_neon_restore(const uint32_t*),em_copy_neon(volatile uint32_t*,const uint32_t*);
extern "C" void em_dim_neon(volatile uint32_t*,const uint32_t*,unsigned);
static void tlb(){uint32_t zero=0;__asm__ volatile("dsb sy\n mcr p15,0,%0,c8,c7,0\n dsb sy\n isb"::"r"(zero):"memory");}
extern "C" uint32_t nes_present(volatile uint8_t *base,uint32_t addr,uint32_t pitch,uint32_t format,uint32_t nonce){
 unsigned effect=ad_get(base+MB_HOST,36),scan=effect&255,scale=scan==4?2:1;
 uint32_t span=nd_span_scaled(addr,pitch,format,scale),*state=(uint32_t*)(base+MB_MMU_STATE);
 uint32_t*l1=(uint32_t*)state[15],table=state[15]+16384+MB_L2_BYTES;
 uint16_t*src=nes_pixels();
 if((format!=3&&format!=4)||!span||!src||table+3072>(uint32_t)base+MB_TABLE_END)return 6;
 unsigned opt=ad_get(base+MB_HOST,32),fast=ad_get(base+MB_HOST,40);
 unsigned fps=effect>>8;
 if(scan>4||!fv_valid(opt)||fast>1||fps>1000)return 6;
 if(!nd_map_scaled(l1,(uint32_t*)table,table,addr,pitch,format,scale))return 6;
 tlb();volatile uint8_t*dst=(volatile uint8_t*)addr;
 uint32_t error=0,row_words[320] __attribute__((aligned(16))),saved[66] __attribute__((aligned(16))),restored[66] __attribute__((aligned(16))),probe[4]={0};
 /* Physical SNES comparisons favour its existing STMIA copy for plain rows.
  * Enable SIMD only where fused dimming pays for the state guard. */
#ifdef EM_SNES
 int neon=fast&&(scan==2||scan==3)&&db_neon_available(probe);
#else
 int neon=fast&&db_neon_available(probe);
#endif
 if(neon)db_neon_snapshot(saved);
 if(ad_get(dst,0)!=nonce||ad_get(dst,span-4)!=(nonce^0x31415926u))error=7;
 else if(scan==4){
  /* 2x nearest-neighbour output, one black separator after each intact row.
   * Unlike Classic at 240p, no logical game row is discarded. */
  for(unsigned y=0;y<240;y++){
   if(ad_get(base,ZZ_STOP)){error=2;break;}
   fv_row(row_words,src,y,opt,format);
   volatile uint32_t*row=(volatile uint32_t*)(dst+y*2*pitch);
   volatile uint32_t*black=(volatile uint32_t*)(dst+(y*2+1)*pitch);
   for(unsigned x=0;x<320;x++){
    uint32_t c=row_words[x],b=0;
    if(fps){int ink=em_fps_ink(x,y,fps-1);if(ink>=0){uint32_t v=ink?0xffffffffu:0xff000000u;c=b=format==3?v:__builtin_bswap32(v);}}
    row[x*2]=row[x*2+1]=c;black[x*2]=black[x*2+1]=b;
   }
  }
 }
 else for(unsigned y=0;y<240;y++){
  if(ad_get(base,ZZ_STOP)){error=2;break;}
  volatile uint32_t*row=(volatile uint32_t*)(dst+y*pitch);
  unsigned level=em_scan_level(scan,y);
  bool vector_dim=neon&&level!=8&&level!=0&&!(fps&&y>=2&&y<11);
  if(fast&&level==0){for(unsigned x=0;x<320;x++)row_words[x]=0;}
  else {
#ifdef EM_SNES
   fv_row(row_words,src,y,opt,format);
#else
   fv_row(row_words,src+y*256,y,opt,format);
#endif
   if(level!=8&&!vector_dim)for(unsigned x=0;x<320;x++)row_words[x]=em_scan_dim(row_words[x],level);
  }
  if(fps&&y>=2&&y<11)for(unsigned x=2;x<42;x++){
   int v=em_fps_ink(x,y,fps-1);if(v>=0){uint32_t c=v?0xffffffffu:0xff000000u;row_words[x]=format==3?c:__builtin_bswap32(c);}}
  if(vector_dim)em_dim_neon(row,row_words,level);
  else if(neon)em_copy_neon(row,row_words);
  else if(fast)ps_copy_row(row,row_words);
  else {
#ifdef EM_SNES
   ps_copy_row(row,row_words);
#else
   for(unsigned x=0;x<320;x++)row[x]=row_words[x];
#endif
  }
 }
 if(neon){db_neon_restore(saved);db_neon_snapshot(restored);
 for(unsigned i=0;i<65;i++)if(saved[i]!=restored[i])error=9;}
 ad_put(base+MB_REPLY,72,neon?0x4e454f4eu:fast?0x53544d49u:0x42415345u);
 __asm__ volatile("dsb sy":::"memory");nd_unmap(l1,addr,span);tlb();return error;
}
