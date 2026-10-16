// GPL-3.0-or-later. Never render while the bitmap is locked: copy only an
// already completed private PPU frame. No ARM access after publishing ACK.
#include "wire.h"
#include "mmu.h"
#include "direct_plan.h"
#include "player_palette.h"
#include "view_fast.h"
#include "nes_api.h"
#include "row_copy_arm.h"
static void tlb(){uint32_t zero=0;__asm__ volatile("dsb sy\n mcr p15,0,%0,c8,c7,0\n dsb sy\n isb"::"r"(zero):"memory");}
extern "C" uint32_t nes_present(volatile uint8_t *base,uint32_t addr,uint32_t pitch,uint32_t format,uint32_t nonce){
 uint32_t span=nd_span(addr,pitch,format),*state=(uint32_t *)(base+MB_MMU_STATE);
 uint32_t *l1=(uint32_t *)state[15],table=state[15]+16384+MB_L2_BYTES;
 uint16_t *src=nes_pixels();
 if((format!=ND_BGRA32 && format!=ND_ARGB32) || !span || !src || table+2048>(uint32_t)base+MB_TABLE_END)return 6;
 if(!nd_map(l1,(uint32_t *)table,table,addr,pitch,format))return 6;
 tlb();volatile uint8_t *dst=(volatile uint8_t *)addr;
 uint32_t error=0,row_words[320],opt=ad_get(base+MB_HOST,32);
 if(!fv_valid(opt)||(format!=3&&format!=4))error=6;
 else if(ad_get(dst,0)!=nonce || ad_get(dst,span-4)!=(nonce^0x31415926u))error=7;
 else for(unsigned y=0;y<240;y++){
  if(ad_get(base,ZZ_STOP)){error=2;break;}
  volatile uint32_t *row=(volatile uint32_t *)(dst+y*pitch);
  fv_row(row_words,src,y,opt,format);
  ps_copy_row(row,row_words);
 }
 // Complete all NC pixel stores, then revoke the temporary mapping BEFORE ACK.
 __asm__ volatile("dsb sy":::"memory");nd_unmap(l1,addr,span);tlb();
 return error;
}
