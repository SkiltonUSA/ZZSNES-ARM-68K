#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "window_scale.h"
#include "direct_plan.h"
int main(void){
 uint32_t row[320],*out=calloc(720*1024+2,4),*small=calloc(320*240,4),l1[4096]={0},l2[1024+1]={0};assert(out&&small);
 for(unsigned x=0;x<320;x++)row[x]=x*1234567u;
 for(unsigned s=1;s<=3;s++){
  memset(out,0x7b,(720*1024+2)*4);
  for(unsigned y=0;y<240;y++){
   for(unsigned x=0;x<320;x++)row[x]=(x*1234567u)^(y*0x01010101u);
   em_window_expand(out+1+y*s*1024,1024,row,s);
  }
  for(unsigned y=0;y<240*s;y++)for(unsigned x=0;x<320*s;x++)assert(out[1+y*1024+x]==(((x/s)*1234567u)^((y/s)*0x01010101u)));
  for(unsigned y=0;y<240*s;y++)for(unsigned x=320*s;x<1024;x++)assert(out[1+y*1024+x]==0x7b7b7b7b);
  assert(out[0]==0x7b7b7b7b&&out[720*1024+1]==0x7b7b7b7b);
  em_window_reduce((uint8_t*)small,(uint8_t*)(out+1),4096,s);
  for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++)assert(small[y*320+x]==((x*1234567u)^(y*0x01010101u)));
  uint32_t addr=0x2ff000,span=nd_span_scaled(addr,4096,ND_BGRA32,s);assert(span);
  l2[1024]=0xdeadbeef;assert(nd_map_scaled(l1,l2,0x8000,addr,4096,ND_BGRA32,s));assert(l2[1024]==0xdeadbeef);
  assert(!nd_map_scaled(l1,l2,0x8000,addr,4096,ND_BGRA32,s));nd_unmap(l1,addr,span);
  for(unsigned i=0;i<4096;i++)assert(!l1[i]);
 }
 assert(!nd_span_scaled(0x2ff000,4096,ND_BGRA32,4));assert(!nd_span_scaled(0x2ff000,1280,ND_BGRA32,3));assert(!nd_span_scaled(ND_APERTURE_END-4096,4096,ND_BGRA32,3));
 assert(em_window_fit(3,1920,1080,32)==3);assert(em_window_fit(3,800,600,32)==2);assert(em_window_fit(3,640,480,32)==1);
 free(out);free(small);puts("PASS 1x/2x/3x pixels, padding, screenshots, screen fit and four-section mapping bounds");
}
