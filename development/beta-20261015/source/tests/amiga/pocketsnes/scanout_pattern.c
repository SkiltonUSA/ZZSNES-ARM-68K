#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "scanout_pattern.h"
int main(void){
 for(unsigned pitch=1280;pitch<=4096;pitch+=4)for(int argb=0;argb<2;argb++){
  size_t size=960u*pitch;uint8_t *p=malloc(size+32);assert(p);memset(p,0xa5,size+32);
  sp_fill(p,pitch,argb);
  for(unsigned s=0;s<4;s++)for(unsigned y=0;y<240;y++){
   for(unsigned x=0;x<320;x++){
    uint8_t *q=p+(s*240+y)*pitch+x*4;
    unsigned offset=(x+y+32-s*8)%32,c=offset<4?112:64;
    assert(q[argb?0:3]==255);for(unsigned b=0;b<3;b++)assert(q[b+(argb?1:0)]==c);
   }
   for(unsigned x=1280;x<pitch;x++)assert(p[(s*240+y)*pitch+x]==0xa5);
  }
  for(unsigned n=0;n<32;n++)assert(p[size+n]==0xa5);
  if(pitch==1280){uint32_t h=sp_hash(p,pitch);p[2000]^=1;assert(sp_hash(p,pitch)!=h);}
  free(p);
 }
 puts("PASS pattern: both byte formats, all aligned pitches, four translated images and padding guards");
}
