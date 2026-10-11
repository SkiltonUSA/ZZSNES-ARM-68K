/* Native ASan/UBSan: descriptor geometry, no clobber, format bytes and guards. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "direct_pixels.h"
static uint32_t l1[4096],l2[514];
int main(void){
 unsigned cases=0;
 for(unsigned fmt=1;fmt<=4;fmt++)for(unsigned pitch=256*nd_bpp(fmt);pitch<=4096;pitch+=4){
  uint32_t span=239*pitch+256*nd_bpp(fmt);
  uint32_t addresses[]={ND_APERTURE_FIRST,0x00fffffcu,0x00f80004u,ND_APERTURE_END-span};
  for(unsigned n=0;n<4;n++){
   uint32_t addr=addresses[n],first=addr>>20,last=(addr+span-1)>>20;
   memset(l1,0,sizeof(l1));memset(l2,0xa5,sizeof(l2));l1[0xfff]=0xdeadbeef;
   assert(nd_span(addr,pitch,fmt)==span);
   assert(nd_map(l1,l2+1,0x10000000,addr,pitch,fmt));
   assert(l2[0]==0xa5a5a5a5 && l2[513]==0xa5a5a5a5);
   for(unsigned i=0;i<4096;i++)assert(l1[i]==(i==0xfff?0xdeadbeef:i>=first&&i<=last?(0x10000000+(i-first)*1024)|1:0));
   for(unsigned i=0;i<512;i++){
    uint32_t page=(first<<20)+i*4096;
    uint32_t want=page>=(addr&~4095u) && page<addr+span ? page|ND_PAGE:0;
    assert(l2[i+1]==want);
   }
   assert(!nd_map(l1,l2+1,0x10000000,addr,pitch,fmt));
   nd_unmap(l1,addr,span);
   for(unsigned i=0;i<4096;i++)assert(l1[i]==(i==0xfff?0xdeadbeef:0));
   assert(!nd_span(ND_APERTURE_END-span+4,pitch,fmt));
   cases++;
  }
 }
 assert(!nd_span(0x1ffffc,768,ND_RGB24));assert(!nd_span(0xffffffff,768,1));
 assert(!nd_span(0x200001,768,1));assert(!nd_span(0x200000,769,1));
 assert(!nd_span(0x200000,764,1));assert(!nd_span(0x200000,4097,1));
 assert(!nd_span(0x200000,1024,0));assert(!nd_span(0x200000,1024,5));
 assert(!nd_map(l1,l2+1,0x10000001,0x200000,1024,3));
 /* Padded rows: check each output byte independently, including emphasis
  * mask and all palette values, plus both destination guard words. */
 uint16_t src[256];uint32_t words[258];
 for(unsigned i=0;i<256;i++)src[i]=(i*13)&511;
 for(unsigned fmt=1;fmt<=4;fmt++){
  memset(words,0xa5,sizeof(words));nd_pack_row(words+1,src,fmt);
  unsigned bpp=nd_bpp(fmt),count=256*bpp;
  for(unsigned i=0;i<count;i++){
   unsigned component=i%bpp;
#ifdef NES_PLAYER
   unsigned c=nes_player_palette[src[i/bpp]&511];
#else
   unsigned c=nes_palette[src[i/bpp]&63];
#endif
   unsigned shift=fmt==1?16-component*8:fmt==2?component*8:fmt==3?component*8:24-component*8;
   unsigned want=(c>>shift)&255;
   assert(((words[1+i/4]>>(8*(i%4)))&255)==want);
  }
  assert(words[0]==0xa5a5a5a5);
  for(unsigned i=1+count/4;i<258;i++)assert(words[i]==0xa5a5a5a5);
 }
 printf("Direct presentation plan: %u mappings, boundary rejection, no clobber, formats and guards PASS\n",cases);
}
