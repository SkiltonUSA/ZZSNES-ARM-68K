#include <assert.h>
#include <stdio.h>
#include "view_fast.h"
static uint16_t src[61440];
static uint32_t row[322];
int main(void){
 for(unsigned format=3;format<=4;format++)for(unsigned opt=0;opt<512;opt++){
  if(!fv_valid(opt))continue;
  for(unsigned i=0;i<61440;i++)src[i]=(uint16_t)(i*257u+opt*43u);
  for(unsigned y=0;y<240;y++){
   row[0]=row[321]=0xdeadbeef;fv_row(row+1,src,y,opt,format);
   for(unsigned x=0;x<320;x++){uint32_t c=fv_color(src,x,y,opt);assert(row[x+1]==(format==3?c:__builtin_bswap32(c)));}
   assert(row[0]==0xdeadbeef&&row[321]==0xdeadbeef);
  }
 }
 puts("PASS LUT renderer: every valid monitor/border/mask setting, both pixel formats, independent pixel reference and row guards");
 return 0;
}
