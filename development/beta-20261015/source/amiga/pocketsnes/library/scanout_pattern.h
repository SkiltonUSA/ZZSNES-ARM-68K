/* GPL-3.0-or-later. Immutable, low-contrast diagonal scanout test images.
 * Four phases form a continuous eight-pixel translation of a 32-pixel repeat.
 * No full-screen luminance alternation. Pixel formats are ARGB/BGRA bytes. */
#pragma once
#include <stdint.h>
static inline uint32_t sp_pixel(unsigned x,unsigned y,unsigned phase,int argb){
 uint32_t c=(((x+y+32u-(phase&3u)*8u)&31u)<4u)?112u:64u;
 return argb?0xff000000u|c*0x010101u:(c*0x010101u)<<8|0xffu;
}
static inline void sp_fill(uint8_t *p,unsigned pitch,int argb){
 unsigned s,y,x;
 for(s=0;s<4;s++)for(y=0;y<240;y++)for(x=0;x<320;x++){
  uint32_t c=sp_pixel(x,y,s,argb);uint8_t *q=p+(s*240+y)*pitch+x*4;
  q[0]=c>>24;q[1]=c>>16;q[2]=c>>8;q[3]=c;
 }
}
static inline uint32_t sp_hash(const uint8_t *p,unsigned pitch){
 uint32_t h=2166136261u;unsigned y,x;
 for(y=0;y<960;y++)for(x=0;x<1280;x++)h=(h^p[y*pitch+x])*16777619u;
 return h;
}
