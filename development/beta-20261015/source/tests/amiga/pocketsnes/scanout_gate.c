#include <assert.h>
#include <stdio.h>
#include "scanout_gate.h"
int main(void){
 struct sg_gate g;
 for(unsigned mode=0;mode<3;mode++)for(int blank=0;blank<2;blank++){
  sg_begin(&g,mode,blank,100);
  if(!mode || (mode==1&&!blank)){assert(sg_poll(&g,blank,101)==1);continue;}
  assert(sg_poll(&g,blank,101)==0);
  if(mode==1){assert(sg_poll(&g,0,102)==1);continue;}
  assert(sg_poll(&g,0,102)==0);assert(sg_poll(&g,0,103)==0);
  assert(sg_poll(&g,1,104)==0);assert(sg_poll(&g,1,105)==0);
  assert(sg_poll(&g,0,106)==1);
 }
 for(unsigned mode=1;mode<=2;mode++){
  sg_begin(&g,mode,1,0xfffffff0u);
  assert(sg_poll(&g,1,20)==0); /* Timer wrap is normal. */
  assert(sg_poll(&g,1,(uint32_t)(0xfffffff0u+50000u))==-1);
 }
 sg_begin(&g,2,0,0);assert(sg_poll(&g,0,50000)==-1);
 sg_begin(&g,2,0,0);assert(sg_poll(&g,1,1)==0);assert(sg_poll(&g,1,50000)==-1);
 puts("PASS blanking gates: active/blank ACK, required transitions, stuck status and timer wrap");
}
