#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "player_palette.h"
#include "view.h"
static uint16_t pixels[256];
static uint32_t output[322];
int main(void){
 for(unsigned f=3;f<=4;f++)for(unsigned opt=0;opt<256;opt++){
  if(!fv_valid(opt))continue;
  for(unsigned x=0;x<256;x++)pixels[x]=(x*7)&511;
  for(unsigned y=0;y<240;y++){
   output[0]=output[321]=0xdeadbeef;
   fv_row(output+1,pixels,y,opt,f);
   for(unsigned x=0;x<320;x++){
    uint32_t c=fv_color(pixels,x,y,opt);
    assert(output[x+1]==(f==3?c:__builtin_bswap32(c)));
   }
   assert(output[0]==0xdeadbeef&&output[321]==0xdeadbeef);
  }
 }
 /* Artwork never fills the live picture or the hidden overscan pixels. */
 for(unsigned t=5;t<=6;t++){
  assert(fv_valid(t*2+1));assert(!fv_valid(7*2+1));
  assert(fv_border(100,100,t*2+1)==0xff000000u);
  assert(fv_border(42,100,t*2+1+16)==0xff000000u);
 }
 puts("PASS NES: all valid themes/geometry/masks, both formats, row guards, black masked edges");
}
