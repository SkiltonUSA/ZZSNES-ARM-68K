#include "core.h"
#include "palette.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
static uint16_t buffers[2][61440];
static int cancel(){return 1;}
int main(int argc,char **argv){
 assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f);uint8_t rom[40977];unsigned n=fread(rom,1,sizeof(rom),f);fclose(f);
 assert(!nes_init(rom,n,buffers[0],buffers[1]));
 for(unsigned i=0;i<120;i++)assert(!nes_frame(0,nullptr));
 uint32_t hash=nes_hash();unsigned colors[512]={};
 for(unsigned i=0;i<61440;i++){assert(nes_pixels()[i]<512);colors[nes_pixels()[i]]++;}
 unsigned distinct=0;for(auto v:colors)if(v)distinct++;
 assert(distinct>=4);printf("CPU_PPU frames=%u cycles=%u hash=%08x colors=%u\n",nes_frames(),nes_cycles(),hash,distinct);
 for(unsigned i=0;i<10;i++)assert(!nes_frame(0x80,nullptr));
 assert(nes_hash()!=hash);printf("CONTROLLER right=10_frames hash=%08x PASS\n",nes_hash());
 assert(nes_frame(0,cancel)==2);puts("CANCEL PASS");
 f=fopen(argv[2],"wb");assert(f);fprintf(f,"P6\n256 240\n255\n");
 for(unsigned i=0;i<61440;i++){unsigned c=nes_palette[nes_pixels()[i]&63];uint8_t rgb[3]={(uint8_t)(c>>16),(uint8_t)(c>>8),(uint8_t)c};fwrite(rgb,1,3,f);}fclose(f);
 rom[6]|=0xf0;assert(nes_init(rom,n,buffers[0],buffers[1])==1);puts("UNSUPPORTED_MAPPER rejected PASS");
}
