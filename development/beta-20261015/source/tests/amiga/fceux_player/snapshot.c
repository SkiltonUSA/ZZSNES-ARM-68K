#include "snapshot_format.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t frame[240*4096],a[SS_BYTES],b[SS_BYTES];
int main(void){
 for(unsigned fmt=3;fmt<=4;fmt++)for(unsigned pitch=1280;pitch<=4096;pitch+=2816){
  memset(frame,0,sizeof(frame));
  for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++){uint8_t*p=frame+y*pitch+x*4;unsigned r=x&255,g=y,b=(x^y)&255;
   if(fmt==3){p[0]=b;p[1]=g;p[2]=r;p[3]=255;}else{p[0]=255;p[1]=r;p[2]=g;p[3]=b;}}
  for(unsigned snes=0;snes<2;snes++)for(unsigned wide=0;wide<2;wide++){
   unsigned w=snes?(wide?216:288):(wide?240:320),x0=(320-w)/2,y0=snes?12:0,h=snes?216:240;
   assert(ss_encode(a,frame,sizeof(frame),pitch,fmt,x0,y0,w,h,123,456,32768));assert(ss_valid(a,SS_BYTES,123,456,32768));assert(ss_nonblank(a));
   for(unsigned y=0;y<112;y++)for(unsigned x=0;x<128;x++){unsigned at=SS_HEADER+(y*128+x)*2,v=((unsigned)a[at]<<8)|a[at+1],want=0;
    if(y>=8&&y<104){unsigned xx=x0+x*w/128,yy=y0+(y-8)*h/96;want=((xx&248)<<8)|((yy&252)<<3)|(((xx^yy)&255)>>3);}assert(v==want);}
  }
 }
 assert(!ss_encode(a,frame,1280,1280,3,0,0,320,240,1,2,3));
 assert(!ss_encode(a,frame,sizeof(frame),1280,2,0,0,320,240,1,2,3));
 assert(!ss_encode(a,frame,sizeof(frame),1280,3,319,0,2,240,1,2,3));
 assert(!ss_encode(a,frame,sizeof(frame),0,3,0,0,320,240,1,2,3));
 assert(!ss_valid(a,SS_BYTES,999,456,32768));assert(!ss_valid(a,SS_BYTES,123,999,32768));assert(!ss_valid(a,SS_BYTES,123,456,1));
 for(unsigned n=0;n<SS_BYTES;n++)assert(!ss_valid(a,n,123,456,32768));
 assert(!ss_valid(a,SS_BYTES+1,123,456,32768));
 memcpy(b,a,SS_BYTES);for(unsigned i=0;i<SS_BYTES;i+=31){b[i]^=1;assert(!ss_valid(b,SS_BYTES,123,456,32768));b[i]^=1;}
 memset(frame,0,sizeof(frame));assert(ss_encode(a,frame,sizeof(frame),1280,3,0,0,320,240,1,2,3));assert(!ss_nonblank(a));
 puts("PASS snapshots: both pixel orders, pitches, monitor/console crops, aspect, bounds, identity, corruption, blank detection");
}
