/* GPL-3.0-or-later. Candidate: exact RGB565 conversion cached as native words.
 * Private allocation-owned BSS only; not part of the emulated/save state.
 * Geometry and pixel values match view.h. No framebuffer/cache-policy change. */
#pragma once
#define fv_row fv_row_original
#include "view.h"
#undef fv_row
static inline void fv_row(uint32_t*dst,const uint16_t*src,unsigned y,unsigned opt,unsigned format){
 static unsigned cached=~0u;
 static int16_t xs[320],ys[240];static unsigned begin,end;
 static uint32_t colors[65536],bezel[240][320];
 unsigned key=opt|(format<<16);
 if(cached!=key){
  for(unsigned p=0;p<65536;p++){uint32_t c=pocket_rgb((uint16_t)p);colors[p]=format==3?c:__builtin_bswap32(c);}
  begin=320;end=0;for(unsigned x=0;x<320;x++){xs[x]=fv_source(x,opt);if(xs[x]>=0){if(begin==320)begin=x;end=x+1;}}
  for(unsigned yy=0;yy<240;yy++){ys[yy]=fv_y(yy,opt);for(unsigned x=0;x<320;x++){uint32_t c=fv_border(x,yy,opt);bezel[yy][x]=format==3?c:__builtin_bswap32(c);}}
  cached=key;
 }
 if(ys[y]<0){for(unsigned x=0;x<320;x++)dst[x]=bezel[y][x];return;}
 const uint16_t*row=src+ys[y]*256;
 for(unsigned x=0;x<begin;x++)dst[x]=bezel[y][x];
 for(unsigned x=begin;x<end;x++)dst[x]=colors[row[xs[x]]];
 for(unsigned x=end;x<320;x++)dst[x]=bezel[y][x];
}
