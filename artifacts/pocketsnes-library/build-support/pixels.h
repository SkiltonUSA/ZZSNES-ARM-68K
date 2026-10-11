/* RGB565 SNES output to the two supported fullscreen 32-bit layouts. */
#pragma once
#include <stdint.h>
#include "direct_plan.h"
static inline uint32_t pocket_rgb(uint16_t p){
 uint32_t r=p>>11,g=(p>>5)&63,b=p&31;
 return 0xff000000u|(((r<<3)|(r>>2))<<16)|(((g<<2)|(g>>4))<<8)|((b<<3)|(b>>2));
}
static inline void nd_pack_row(uint32_t *dst,const uint16_t *src,uint32_t format){
 for(unsigned x=0;x<256;x++){uint32_t c=pocket_rgb(src[x]);dst[x]=format==ND_BGRA32?c:__builtin_bswap32(c);}
}
