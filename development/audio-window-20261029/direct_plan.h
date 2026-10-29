/* GPL-3.0-or-later. Temporary NC/XN mapping of an OS-owned, locked bitmap.
 * This validates geometry/aperture only; ownership comes from the host's
 * P96 allocation and held lock, challenged at BOTH ends before any write. */
#pragma once
#include <stdint.h>
#define ND_RGB24 1u
#define ND_BGR24 2u
#define ND_BGRA32 3u
#define ND_ARGB32 4u
#define ND_APERTURE_FIRST 0x00200000u
#define ND_APERTURE_END 0x041f0000u
#define ND_PAGE 0x453u /* normal NC, shared, privileged RW, XN */
static inline uint32_t nd_bpp(uint32_t format){return format==1||format==2?3:format==3||format==4?4:0;}
static inline uint32_t nd_span_scaled(uint32_t addr,uint32_t pitch,uint32_t format,unsigned scale){
 uint32_t bpp=nd_bpp(format),span;
 if((scale<1||scale>3) || !bpp || (addr&3) || (pitch&3) || pitch<320*scale*bpp || pitch>4096)return 0;
 span=(240*scale-1)*pitch+320*scale*bpp;
 if(addr<ND_APERTURE_FIRST || addr>ND_APERTURE_END-span)return 0;
 return span;
}
static inline int nd_map_scaled(uint32_t *l1,uint32_t *l2,uint32_t table,uint32_t addr,uint32_t pitch,uint32_t format,unsigned scale){
 uint32_t span=nd_span_scaled(addr,pitch,format,scale),first=addr>>20,last,i,page;
 if(!span || (table&1023))return 0;
 last=(addr+span-1)>>20;
 if(last-first>3)return 0;
 for(i=first;i<=last;i++)if(l1[i])return 0; /* never replace another mapping */
 for(i=0;i<(last-first+1)*256;i++)l2[i]=0;
 for(page=addr&~4095u;page<addr+span;page+=4096)
  l2[((page>>20)-first)*256+((page>>12)&255)]=page|ND_PAGE;
 for(i=first;i<=last;i++)l1[i]=(table+(i-first)*1024)|1;
 return 1;
}
static inline void nd_unmap(uint32_t *l1,uint32_t addr,uint32_t span){
 uint32_t first=addr>>20,last=(addr+span-1)>>20;
 for(uint32_t i=first;i<=last;i++)l1[i]=0;
}

static inline uint32_t nd_span(uint32_t a,uint32_t p,uint32_t f){return nd_span_scaled(a,p,f,1);}
static inline int nd_map(uint32_t*l1,uint32_t*l2,uint32_t t,uint32_t a,uint32_t p,uint32_t f){return nd_map_scaled(l1,l2,t,a,p,f,1);}
