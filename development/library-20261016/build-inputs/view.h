/* Procedural SNES bezel and user-supplied side panels; see amiga/borders/. */
#pragma once
#include <stdint.h>
#include "border_art.h"
#include "pixels.h"
static inline int fv_valid(unsigned opt){return opt<512&&((opt>>1)&7)<7&&((opt>>4)&3)<3&&((opt>>6)&3)<3;}
static inline unsigned fv_width(unsigned opt){return opt&1?216:288;}
static inline int fv_source(unsigned x,unsigned opt){unsigned w=fv_width(opt),left=(320-w)/2;if(x<left||x>=left+w)return -1;unsigned sx=(x-left)*256/w,m=((opt>>4)&3)*8;return sx<m||sx>=256-m?-1:(int)sx;}
static inline int fv_y(unsigned y,unsigned opt){if(y<12||y>=228)return -1;unsigned h=opt&256?239:224,sy=(y-12)*h/216,m=((opt>>6)&3)*8;return sy<m||sy>=h-m?-1:(int)sy+(h==224?8:0);}
static inline uint32_t fv_border(unsigned x,unsigned y,unsigned opt){
 unsigned t=(opt>>1)&7;if(t>=5)return border_art(x,y,t,fv_width(opt));if(t==0)return 0xff000000;if(t==1)return 0xffffffff;
 uint32_t body=t==4?0xff30343c:0xffbebec6,edge=t==4?0xff181c24:0xff8c8c98;
 if(y==0||y==239||x==0||x==319)return edge;
 /* Cartridge slot, sculpted housing and side cooling vents. */
 if(y>=3&&y<=7&&x>=82&&x<238)return y==3?edge:0xff252630;
 if((x<12||x>=308)&&y>28&&y<200&&y%7<2)return edge;
 if(y>=232&&x>=36&&x<63)return t==3?0xff786cac:0xff625097; /* POWER */
 if(y>=232&&x>=73&&x<100)return 0xff9595a4; /* RESET */
 /* Four face-button colours echo US SNES or Super Famicom controllers. */
 const unsigned cx[4]={274,289,280,295},cy[4]={232,232,236,236};
 const uint32_t pal[4]={0xff287cc0,0xffd4b92a,0xff37a454,0xffc64545};
 for(unsigned i=0;i<4;i++){int dx=(int)x-(int)cx[i],dy=(int)y-(int)cy[i];if(dx*dx+dy*dy<=9)return t==3?pal[i]:i<2?0xffa69ad5:0xff66549e;}
 if((y==10||y==229)&&x>14&&x<306)return edge;
 return body;
}
static inline uint32_t fv_color(const uint16_t*src,unsigned x,unsigned y,unsigned opt){int sx=fv_source(x,opt),sy=fv_y(y,opt);return sx<0||sy<0?fv_border(x,y,opt):pocket_rgb(src[sy*256+sx]);}
/* Cache geometry and the static bezel; division never runs per game pixel.
 * This storage belongs to the current core allocation and is not serialized. */
static inline void fv_row(uint32_t*dst,const uint16_t*src,unsigned y,unsigned opt,unsigned format){
 static unsigned cached=~0u;static int16_t xs[320],ys[240];static uint32_t bezel[240][320];
 if(cached!=opt){for(unsigned x=0;x<320;x++)xs[x]=fv_source(x,opt);
  for(unsigned yy=0;yy<240;yy++){ys[yy]=fv_y(yy,opt);for(unsigned x=0;x<320;x++)bezel[yy][x]=fv_border(x,yy,opt);}cached=opt;}
 const uint16_t *row=ys[y]<0?src:src+ys[y]*256;
 for(unsigned x=0;x<320;x++){uint32_t c=ys[y]<0||xs[x]<0?bezel[y][x]:pocket_rgb(row[xs[x]]);dst[x]=format==3?c:__builtin_bswap32(c);}
}
