/* GPL-3.0-or-later. Candidate: exact RGB565 conversion cached as native words.
 * Private allocation-owned BSS only; not part of the emulated/save state.
 * Geometry and pixel values match view.h. No framebuffer/cache-policy change. */
#pragma once
#define fv_row fv_row_original
#include "view.h"
#undef fv_row
#if defined(__arm__)
struct fv_group {uint32_t offset,a[4],b[4],weight[4];};
extern "C" void em_blend_neon(uint32_t*,const uint32_t*,const fv_group*,unsigned);
static unsigned fv_neon;
#endif
static inline void fv_row(uint32_t*dst,const uint16_t*src,unsigned y,unsigned opt,unsigned format){
 static unsigned cached=~0u;
 static int16_t xs[320],ys[240];static unsigned begin,end;
 static uint16_t first[320],second[320],weight[320];
#if defined(__arm__)
 static uint32_t source_colors[263] __attribute__((aligned(16)));
 static fv_group groups[80];
#endif
 static uint32_t colors[65536],bezel[240][320];
 unsigned key=opt|(format<<16);
 if(cached!=key){
  for(unsigned p=0;p<65536;p++){uint32_t c=pocket_rgb((uint16_t)p);colors[p]=format==3?c:__builtin_bswap32(c);}
  begin=320;end=0;for(unsigned x=0;x<320;x++){xs[x]=fv_source(x,opt);if(xs[x]>=0){if(begin==320)begin=x;end=x+1;}}
  if(opt&512){
   for(unsigned x=begin;x<end;x++){unsigned a,b,w;fv_sample(x,opt,&a,&b,&w);first[x]=a;second[x]=b;weight[x]=w;}
#if defined(__arm__)
   for(unsigned x=begin;x+4<=end;x+=4){fv_group &g=groups[(x-begin)/4];g.offset=first[x]*4;
    for(unsigned i=0;i<4;i++){g.a[i]=0x03020100u+(first[x+i]-first[x])*0x04040404u;
     g.b[i]=0x03020100u+(second[x+i]-first[x])*0x04040404u;g.weight[i]=weight[x+i]*0x01010101u;}}
#endif
  }
  for(unsigned yy=0;yy<240;yy++){ys[yy]=fv_y(yy,opt);for(unsigned x=0;x<320;x++){uint32_t c=fv_border(x,yy,opt);bezel[yy][x]=format==3?c:__builtin_bswap32(c);}}
  cached=key;
 }
 if(ys[y]<0){for(unsigned x=0;x<320;x++)dst[x]=bezel[y][x];return;}
 const uint16_t*row=src+ys[y]*256;
 for(unsigned x=0;x<begin;x++)dst[x]=bezel[y][x];
 if(opt&512){unsigned x=begin;
#if defined(__arm__)
  if(fv_neon){for(unsigned i=0;i<256;i++)source_colors[i]=colors[row[i]];
   for(unsigned i=256;i<263;i++)source_colors[i]=source_colors[255];
   em_blend_neon(dst+begin,source_colors,groups,end-begin);x=begin+((end-begin)&~3u);}
#endif
  for(;x<end;x++){uint32_t c=fv_blend(pocket_rgb(row[first[x]]),pocket_rgb(row[second[x]]),weight[x]);dst[x]=format==3?c:__builtin_bswap32(c);}
 }else for(unsigned x=begin;x<end;x++)dst[x]=colors[row[xs[x]]];
 for(unsigned x=end;x<320;x++)dst[x]=bezel[y][x];
}
