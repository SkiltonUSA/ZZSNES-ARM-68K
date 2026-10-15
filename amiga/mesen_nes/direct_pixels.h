/* GPL-3.0-or-later. Independent byte-format packing for the ARM presenter.
 * Words are little-endian ARM stores. No dependency on Xanxi/PicoDrive code. */
#pragma once
#include "direct_plan.h"
#include "palette.h"
#ifdef NES_PLAYER
#include "player_palette.h"
#endif
static inline void nd_pack_row(uint32_t *words,const uint16_t *src,uint32_t format){
 uint32_t bpp=nd_bpp(format),acc=0,shift=0,out=0;
 for(unsigned x=0;x<256;x++){
  #ifdef NES_PLAYER
  uint32_t c=nes_player_palette[src[x]&511];
  #else
  uint32_t c=nes_palette[src[x]&63];
  #endif
  uint8_t bytes[4];
  if(format==ND_RGB24){bytes[0]=c>>16;bytes[1]=c>>8;bytes[2]=c;}
  else if(format==ND_BGR24){bytes[0]=c;bytes[1]=c>>8;bytes[2]=c>>16;}
  else if(format==ND_BGRA32){bytes[0]=c;bytes[1]=c>>8;bytes[2]=c>>16;bytes[3]=255;}
  else {bytes[0]=255;bytes[1]=c>>16;bytes[2]=c>>8;bytes[3]=c;}
  for(unsigned i=0;i<bpp;i++){
   acc|=(uint32_t)bytes[i]<<shift;shift+=8;
   if(shift==32){words[out++]=acc;acc=0;shift=0;}
  }
 }
}
