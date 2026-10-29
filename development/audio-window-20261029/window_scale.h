/* GPL-3.0-or-later. Integer window scaling; never accesses Workbench storage. */
#ifndef EM_WINDOW_SCALE_H
#define EM_WINDOW_SCALE_H
#include <stdint.h>
static unsigned em_window_fit(unsigned requested,unsigned width,unsigned height,unsigned borders){
 while(requested>1&&(320*requested+16>width||240*requested+borders>height))requested--;
 return requested;
}
static void em_window_expand(volatile uint32_t *dst,unsigned pitch,const uint32_t *row,unsigned scale){
 for(unsigned dy=0;dy<scale;dy++)
  for(unsigned x=0;x<320;x++)
   for(unsigned dx=0;dx<scale;dx++)dst[dy*pitch+x*scale+dx]=row[x];
}
static void em_window_reduce(uint8_t *dst,const uint8_t *src,unsigned pitch,unsigned scale){
 for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++)
  for(unsigned b=0;b<4;b++)dst[(y*320+x)*4+b]=src[y*scale*pitch+x*scale*4+b];
}
#endif
