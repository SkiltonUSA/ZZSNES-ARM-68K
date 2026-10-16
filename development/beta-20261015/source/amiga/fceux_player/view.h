/* GPL-3.0-or-later. Procedural themes plus user-supplied side-panel themes; see amiga/borders/.
 * 320x240 scanout stretched by a 4:3 or 16:9 monitor. Game stays physical 4:3.
 * Option bits: monitor[0], theme[1..3], horizontal mask[4..5], vertical mask[6..7] (0/8/16 pixels). */
#pragma once
#include <stdint.h>
#include "border_art.h"
static inline int fv_valid(unsigned opt){return opt<256 && ((opt>>1)&7)<7 && ((opt>>4)&3)<3 && ((opt>>6)&3)<3;}
static inline unsigned fv_width(unsigned opt){return (opt&1)?240:320;}
static inline int fv_source(unsigned x,unsigned opt){
 unsigned width=fv_width(opt),left=(320-width)/2,sx;
 if(x<left||x>=left+width)return -1;
 /* Constant-ratio divisions compile to multiplies/shifts on Cortex-A9. */
 sx=(opt&1)?(x-left)*16/15:(x-left)*4/5;
 if(sx<((opt>>4)&3)*8||sx>=256-((opt>>4)&3)*8)return -1;
 return (int)sx;
}
static inline uint32_t fv_border(unsigned x,unsigned y,unsigned opt){
 unsigned theme=(opt>>1)&7;
 if(theme>=5)return border_art(x,y,theme,fv_width(opt));
 if(theme==0)return 0xff000000u;
 if(theme==1)return 0xffffffffu;
 if(theme==2){ /* Console grey, vents and red pinstripe. */
  if(y>=190&&y<196)return 0xffc82b32u;
  if(y>28&&y<160&&(x%8)<3)return 0xff282828u;
  return 0xffb8b8b0u;
 }
 if(theme==3){ /* Cream and burgundy. */
  if(y<24||y>212)return 0xff8c2438u;
  if(y>180&&y<185)return 0xffac903cu;
  return 0xffeee4c8u;
 }
 {unsigned h=(x*1973u+y*9277u+89173u);h=(h^(h>>7))*1103515245u;
  return (h%211u==0)?0xffe4e8ffu:0xff101827u;}
}
static inline uint32_t fv_color(const uint16_t *src,unsigned x,unsigned y,unsigned opt){
 int sx=fv_source(x,opt);unsigned my=((opt>>6)&3)*8;return (sx<0||y<my||y>=240-my)?fv_border(x,y,opt):nes_player_palette[src[sx]&511];
}
static inline void fv_row(uint32_t *dst,const uint16_t *src,unsigned y,unsigned opt,unsigned format){
 /* Prepare scaling once per option change, not once per pixel or scanline. */
 static uint16_t map[320];static unsigned previous=~0u,begin,end;
 static uint32_t panels[240][80];
 unsigned art=((opt>>1)&7)>=5;
 unsigned x,my=((opt>>6)&3)*8;
 if(previous!=opt){begin=320;end=0;for(x=0;x<320;x++){int sx=fv_source(x,opt);
   if(sx>=0){map[x]=(uint16_t)sx;if(begin==320)begin=x;end=x+1;}}
  if(art)for(unsigned yy=0;yy<240;yy++)for(unsigned xx=0;xx<80;xx++)
   panels[yy][xx]=fv_border(xx<40?xx:xx+240,yy,opt);
  previous=opt;
 }
 if(y<my||y>=240-my){for(x=0;x<320;x++){uint32_t c=art?(x<40?panels[y][x]:x>=280?panels[y][x-240]:0xff000000u):fv_border(x,y,opt);dst[x]=format==3?c:__builtin_bswap32(c);}return;}
 for(x=0;x<begin;x++){uint32_t c=art?(x<40?panels[y][x]:x>=280?panels[y][x-240]:0xff000000u):fv_border(x,y,opt);dst[x]=format==3?c:__builtin_bswap32(c);}
 if(format==3)for(x=begin;x<end;x++)dst[x]=nes_player_palette[src[map[x]]&511]|0xff000000u;
 else for(x=begin;x<end;x++)dst[x]=__builtin_bswap32(nes_player_palette[src[map[x]]&511]|0xff000000u);
 for(x=end;x<320;x++){uint32_t c=art?(x<40?panels[y][x]:x>=280?panels[y][x-240]:0xff000000u):fv_border(x,y,opt);dst[x]=format==3?c:__builtin_bswap32(c);}
}
