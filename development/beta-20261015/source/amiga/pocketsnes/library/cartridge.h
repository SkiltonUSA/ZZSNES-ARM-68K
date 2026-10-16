#pragma once
#include <stdint.h>
#include <string.h>
/* Conservative initial compatibility: standard NTSC/PAL LoROM/HiROM, no coprocessors.
 * Both headerless and 512-byte copier-header files are normalized by the host. */
static int snes_header(const uint8_t*p,unsigned n){
 if(n<32768||n>0x400000||(n&32767))return -1;
 for(unsigned i=0;i<2;i++){
  unsigned h=i?0xffc0:0x7fc0;if(h+64>n)continue;
  unsigned map=p[h+21],type=p[h+22],sum=p[h+28]|p[h+29]<<8,check=p[h+30]|p[h+31]<<8;
  /* This early cartridge uses a 22-byte title: its final S occupies the
   * normal map byte. Accept only this bounded LoROM signature; no ROM edit. */
  int contra=!i&&n==0x100000&&map==0x53&&type==0&&p[h+23]==10&&p[h+24]==0&&p[h+25]==1&&
   !memcmp(p+h,"CONTRA3 THE ALIEN WARS",22);
  if((map==0x20+i||map==0x30+i||contra)&&type<=2&&p[h+25]<=12&&p[h+24]<=7&&p[h+23]>=5&&p[h+23]<=12&&
    (sum^check)==65535&&(p[h+61]&128))return (int)h;
 }return -1;
}
static unsigned snes_skip(unsigned n){return (n&32767)==512?512:0;}
static uint32_t snes_hash(const uint8_t*p,unsigned n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
static uint32_t snes_hash2(const uint8_t*p,unsigned n){uint32_t h=0x9e3779b9u;while(n--)h=(h+*p++)*65599u;return h;}
static int nes_cartridge(const uint8_t*p,unsigned n){return snes_header(p,n);}

static int snes_pal(const uint8_t*p,unsigned n){int h=snes_header(p,n);return h>=0&&p[h+25]>=2;}
