/* GPL-3.0-or-later. Specialise the two 32-bit formats used by fullscreen.
 * Palette words are ARGB values; ARM stores words little-endian. No change
 * to colours, emphasis, alpha, mapping, ownership or cache policy. */
#pragma once
#define nd_pack_row nd_pack_row_generic
#include "direct_pixels.h"
#undef nd_pack_row
static inline void nd_pack_row(uint32_t *words,const uint16_t *src,uint32_t format){
 if(format==ND_BGRA32){
  for(unsigned x=0;x<256;x++)words[x]=nes_player_palette[src[x]&511]|0xff000000u;
 }else if(format==ND_ARGB32){
  for(unsigned x=0;x<256;x++)words[x]=__builtin_bswap32(nes_player_palette[src[x]&511]|0xff000000u);
 }else nd_pack_row_generic(words,src,format);
}
