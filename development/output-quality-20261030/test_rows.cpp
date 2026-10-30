#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <cstring>
#include "view_fast.h"
#include "hires_view.h"
int main(){
 for(unsigned region: {0u,256u})for(unsigned mask=0;mask<3;mask++){
  unsigned opt=region|(mask<<6),h=region?239:224,m=mask*8,seen[240]={0};int last=-1;
  for(unsigned y=0;y<240;y++){int s=fv_y(y,opt);if(s<0)continue;unsigned logical=s-(region?0:8);assert(logical>=m&&logical<h-m);seen[logical]++;if(last>=0)assert(s==last+1);last=s;}
  for(unsigned i=0;i<h;i++)assert(seen[i]==(i>=m&&i<h-m));
  for(unsigned y=0;y<480;y++){int s=ps_hires_y(y,opt),normal=fv_y(y/2,opt);assert(s==(normal<0?-1:2*normal+(y&1)));}
 }
 /* Vertically moving checker pattern: optimized row and independent scalar
  * conversion agree for every pixel, format, mask, overscan and smooth option. */
 static uint16_t src[256*240];uint32_t row[320];
 for(unsigned shift=0;shift<32;shift++){
  for(unsigned y=0;y<240;y++)for(unsigned x=0;x<256;x++)src[y*256+x]=((y+shift)*137+x*31)&65535;
  for(unsigned opt: {0u,80u,160u,256u,512u,592u,768u})for(unsigned format: {3u,4u})for(unsigned y=0;y<240;y++){
   fv_row(row,src,y,opt,format);for(unsigned x=0;x<320;x++){uint32_t c=fv_color(src,x,y,opt);if(format==4)c=__builtin_bswap32(c);assert(row[x]==c);}
  }
 }
 puts("PASS intact NTSC/overscan rows, edge masks, hires row alignment and 32 scrolling offsets in scalar/optimized paths");
}
