/* GPL-3.0-or-later. Exhaustive 16-bit indices and all four formats. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#ifndef NES_PLAYER
#define NES_PLAYER
#endif
#include "fast_pixels.h"
int main(void){
 uint16_t indices[256];uint32_t old[258],fast[258];
 for(unsigned format=1;format<=4;format++)for(unsigned base=0;base<65536;base+=256){
  for(unsigned i=0;i<256;i++)indices[i]=(uint16_t)(base+i);
  memset(old,0xa5,sizeof(old));memset(fast,0xa5,sizeof(fast));
  nd_pack_row_generic(old+1,indices,format);nd_pack_row(fast+1,indices,format);
  assert(!memcmp(old,fast,sizeof(old)));
 }
 puts("PASS all 65536 input indices, four formats, alpha/emphasis and output guards match generic packer");
}
