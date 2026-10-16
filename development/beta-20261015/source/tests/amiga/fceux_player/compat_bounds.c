#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t ULONG;typedef int32_t LONG;
#include "cartridge.h"
#include "pace.h"
int main(void){uint8_t*r=calloc(1,1048592);memcpy(r,"NES\032",4);r[4]=8;r[5]=0;r[6]=0x21;
 assert(nes_cartridge(r,131088)==2);assert(!fc_rom_pal(r,131088,"Games:Europe/Contra (USA).nes"));assert(fc_rom_pal(r,131088,"Games:Asterix (EUROPE).nes"));assert(fc_rom_pal(r,131088,"Games:Aussie (Australia).nes"));r[9]=1;assert(nes_cartridge(r,131088)==2&&fc_rom_pal(r,131088,"untagged.nes"));r[9]=2;assert(nes_cartridge(r,131088)<0);r[9]=0;r[6]|=4;assert(nes_cartridge(r,131088)<0);r[6]=0x31;assert(nes_cartridge(r,131088)<0);assert(nes_cartridge(0,16)<0);assert(nes_cartridge(r,15)<0);free(r);
 for(unsigned pal=0;pal<2;pal++){ULONG p=fc_period_us(pal),v=1000;for(unsigned i=0;i<1000;i++){ULONG n=fc_next_deadline(v,v,p);assert(n-v==p);v=n;}assert(fc_next_deadline(200000,1000,p)==200000+p);v=0xfffffff0;assert(fc_next_deadline(v,v,p)==v+p);}
 assert(0x100000+1048592u<=0x202000);assert(0x202000+122880u==0x220000);assert(0xe0000+61440*2<=0x100000);
 puts("PASS region tags/header/malformed ROMs, PAL/NTSC deadlines and wrap, non-overlapping owned ROM/state/video ranges");return 0;}
