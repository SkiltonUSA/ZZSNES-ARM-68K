/* Source-level guards for region loading, retained geometry and LUT keys. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "player.c"
#include "view_fast.h"
void pocket_heap_init(void){}
int main(void){
 uint8_t*rom=calloc(1,0x100000);unsigned h=0x7fc0;
 rom[h+21]=0x20;rom[h+23]=10;rom[h+28]=rom[h+29]=255;rom[h+61]=128;
 for(unsigned region=0;region<=12;region++){rom[h+25]=region;assert(snes_header(rom,0x100000)==h);assert(snes_pal(rom,0x100000)==(region>=2));}
 rom[h+25]=13;assert(snes_header(rom,0x100000)<0);rom[h+25]=2;rom[h+22]=3;assert(snes_header(rom,0x100000)<0);rom[h+22]=0;
 rom[h+21]=0x53;assert(snes_header(rom,0x100000)<0);memcpy(rom+h,"CONTRA3 THE ALIEN WARS",22);rom[h+25]=1;assert(snes_header(rom,0x100000)==h);
 rom[h]='X';assert(snes_header(rom,0x100000)<0);rom[h]='C';rom[h+25]=2;assert(snes_header(rom,0x100000)<0);
 assert(snes_skip(0x100200)==512&&!snes_skip(0x100000));free(rom);
 uint16_t*src=malloc(256*239*2);for(unsigned i=0;i<256*239;i++)src[i]=(i*13)^0xa563;
 video(src,256,239,512);assert(!failed&&ps_video_height()==239);assert(!memcmp(picture,src,256*239*2));for(unsigned x=0;x<256;x++)assert(picture[239*256+x]==0);
 video(src,256,224,512);assert(!failed&&ps_video_height()==224);assert(!memcmp(picture+8*256,src,256*224*2));for(unsigned x=0;x<8*256;x++)assert(!picture[x]&&!picture[232*256+x]);
 uint32_t a[322],b[320];unsigned opts[]={0,2,4,6,8,21,85,256,258,260,262,264,277,341};
 for(unsigned i=0;i<sizeof(opts)/sizeof(*opts);i++)for(unsigned fmt=3;fmt<=4;fmt++){
  unsigned opt=opts[i];assert(fv_valid(opt));a[0]=a[321]=0x12345678;
  for(unsigned y=0;y<240;y++){fv_row(a+1,picture,y,opt,fmt);fv_row_original(b,picture,y,opt,fmt);assert(!memcmp(a+1,b,sizeof(b)));assert(a[0]==0x12345678&&a[321]==0x12345678);int sy=fv_y(y,opt);assert(sy<0||(unsigned)sy<240);}
 }
 video(src,256,240,512);assert(failed);assert(ps_picture_height(0)==224);free(src);
 puts("PASS PAL/NTSC and Contra bounds; 224/239 transitions; LUT/reference/endian/row guards; legacy state height");return 0;
}
