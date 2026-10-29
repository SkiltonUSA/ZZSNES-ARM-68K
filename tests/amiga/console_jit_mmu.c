#include "mmu_plan.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static uint32_t l1[4096],l2[MB_L2_BYTES/4];
static void separate(uint32_t a,uint32_t an,uint32_t b,uint32_t bn){assert(a+an<=b||b+bn<=a);}
int main(void){
 separate(MB_WITNESS-32,1088,NES_OUTPUT,NES_OUTPUT_BYTES);
 separate(MB_WITNESS-32,1088,NES_BUFFER_A,NES_PIXELS*2);
 separate(MB_WITNESS-32,1088,NES_BUFFER_B,NES_PIXELS*2);
 separate(MB_WITNESS-32,1088,NES_AUDIO,65536);
 separate(MB_WITNESS-32,1088,NES_ROM,NES_ROM_LIMIT);
 separate(MB_WITNESS-32,1088,NES_SAVE,NES_SAVE_LIMIT);
 separate(MB_WITNESS-32,1088,MB_PUBLIC_BEGIN,MB_PUBLIC_END-MB_PUBLIC_BEGIN);
 assert(CJ_BYTES==0x2000000u);
 for(uint32_t mis=0;mis<0x100000;mis+=64){
  uint32_t base=0x12000000+mis,t=mb_align_up(base+MB_TABLE_BEGIN,16384);
  assert(mb_make_tables(base,l1,t,l2,t+16384));
  separate(base+MB_WITNESS-32,1088,t,16384+MB_L2_BYTES);
  uint32_t code=mb_align_up(base+CJ_BEGIN,4096),end=code+0x2000000;
  assert(code>=base+MB_TABLE_END&&end<base+MB_ALLOCATION-0x30000);
  unsigned executable=0;
  for(uint32_t page=base&~4095u;page<base+MB_ALLOCATION;page+=4096){
   unsigned attr=mb_page_attr(base,page);
   if(page>=code&&page<end){assert(attr==MB_PAGE_PRIVATE);executable+=4096;}
   else if(page>=base+MB_TABLE_BEGIN)assert(attr&1u);
   if(page<base||page+4096>base+MB_ALLOCATION)assert(attr== (MB_PAGE_SHARED | (page>=base+MB_TABLE_BEGIN)));
  }
  assert(executable==0x2000000);
 }
 puts("PASS 32 MiB executable region, stack/shared XN, table bounds at 16384 alignments");
}
