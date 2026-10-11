#include <assert.h>
#include <stdio.h>
#include "pixels.h"
int main(void){uint16_t src[256];uint32_t out[258];
 for(unsigned start=0;start<65536;start+=256){
  for(unsigned i=0;i<256;i++)src[i]=start+i;
  for(unsigned fmt=3;fmt<=4;fmt++){
   out[0]=out[257]=0xdeadbeef;nd_pack_row(out+1,src,fmt);assert(out[0]==0xdeadbeef&&out[257]==0xdeadbeef);
   for(unsigned i=0;i<256;i++){unsigned p=src[i],r=(p>>11)&31,g=(p>>5)&63,b=p&31;unsigned c=0xff000000u|((r*8+r/4)<<16)|((g*4+g/16)<<8)|(b*8+b/4);if(fmt==4)c=((c&255)<<24)|((c&0xff00)<<8)|((c>>8)&0xff00)|(c>>24);assert(out[i+1]==c);}
  }
 }puts("PASS all 65536 RGB565 colours in both 32-bit formats; output guards intact");}
