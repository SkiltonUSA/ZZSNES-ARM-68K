// GPL-3.0-or-later. Compare row splitting against the scalar colour oracle.
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
#include "view_fast.h"
int main(){static uint16_t src[256*240];uint32_t dst[322];unsigned count=0;
 for(unsigned i=0;i<256*240;i++)src[i]=(uint16_t)(i*37+i/17);
 for(unsigned opt=0;opt<1024;opt++)if(fv_valid(opt))for(unsigned fmt=3;fmt<=4;fmt++)for(unsigned y: {0u,11u,12u,23u,28u,119u,222u,227u,239u}){
  dst[0]=dst[321]=0x12345678;fv_row(dst+1,src,y,opt,fmt);assert(dst[0]==0x12345678&&dst[321]==0x12345678);
  for(unsigned x=0;x<320;x++){uint32_t c=fv_color(src,x,y,opt);if(fmt==4)c=__builtin_bswap32(c);assert(dst[x+1]==c);count++;}}
 printf("PASS %u SNES output pixels: all options, both formats, PAL height, masks, borders and guards\n",count);
}
