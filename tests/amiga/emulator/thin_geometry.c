/* Original bounded framebuffer-map tests, no hardware access. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "direct_plan.h"
int main(void){
 uint32_t l1[4096]={0},guard[770],*l2=guard+1;
 uint32_t address=0x02fff800,pitch=2576,table=0x14040000;
 memset(guard,0xa5,sizeof(guard));
 assert(nd_span_scaled(address,pitch,ND_BGRA32,2)==479*pitch+2560);
 assert(!nd_span_scaled(address,1280,ND_BGRA32,2));
 assert(!nd_span_scaled(address,pitch,ND_BGRA32,0));
 assert(!nd_span_scaled(address,pitch,ND_BGRA32,3));
 assert(!nd_span_scaled(ND_APERTURE_END-2560,pitch,ND_BGRA32,2));
 l1[0x31]=0x1234;
 assert(!nd_map_scaled(l1,l2,table,address,pitch,ND_BGRA32,2));
 assert(!l1[0x2f]&&!l1[0x30]&&l1[0x31]==0x1234);
 l1[0x31]=0;
 assert(nd_map_scaled(l1,l2,table,address,pitch,ND_BGRA32,2));
 assert(l1[0x2f]==(table|1)&&l1[0x30]==((table+1024)|1)&&l1[0x31]==((table+2048)|1));
 assert(guard[0]==0xa5a5a5a5&&guard[769]==0xa5a5a5a5);
 nd_unmap(l1,address,nd_span_scaled(address,pitch,ND_BGRA32,2));
 for(unsigned i=0;i<4096;i++)assert(!l1[i]);
 puts("PASS Thin geometry: bounded span, three sections, occupied mapping rejection, table guards and unmap");
}
