/* Original synthetic instruction/mapping tests; no game bytes. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snes9x.h"
#include "memmap.h"
#include "ppu.h"
#include "cpuexec.h"
#include "getset.h"
#include "fxemu.h"
#include "fxinst.h"
#include "libretro.h"
extern FxRegs_s GSU;
int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*);
void pocket_player_close(void);
void S9xResetSuperFX(void);
void pocket_heap_init(void){}
static void start(unsigned command,unsigned bank,unsigned pc){
 /* IWT R0,#command ; MERGE ; STOP ; NOP. */
 uint8_t program[]={0xf0,command,command>>8,0x70,0,1};
 unsigned offset=bank<0x40?(bank<<15)+(pc&0x7fff):((bank-0x40)<<16)+pc;
 if(bank==0x70)memcpy(Memory.SRAM+pc,program,sizeof(program));
 else memcpy(Memory.ROM+offset,program,sizeof(program));
 S9xResetSuperFX();S9xSetByte(bank,0x007034);
 S9xSetByte(pc&255,0x80701e);S9xSetByte(pc>>8,0x80701f);
 assert(S9xGetByte(0x00701e)==0&&S9xGetByte(0x00701f)==0);
 assert(!(Memory.FillRAM[0x3030]&FLG_G));
 assert(!(Memory.FillRAM[0x3031]&0x80));
 assert(!(CPU.IRQActive&GSU_IRQ_SOURCE));
}
int main(void){
 unsigned n=0x400000;uint8_t*rom=malloc(n);assert(rom);
 for(unsigned i=0;i<n;i++)rom[i]=(i>>16)^0x5a;
 memcpy(rom+0x7fc0,"ORIGINAL FX3 TEST    ",20);
 rom[0x7fd5]=0x20;rom[0x7fd6]=0x17;rom[0x7fd7]=12;rom[0x7fd8]=7;rom[0x7fd9]=1;
 rom[0x7fdc]=rom[0x7fdd]=255;rom[0x7fde]=rom[0x7fdf]=0;
 rom[0x7ffc]=0;rom[0x7ffd]=0x80;rom[0]=0x80;rom[1]=0xfe;
 for(unsigned type=0x17;type<=0x18;type++){
  rom[0x7fd6]=type;assert(!nes_init(rom,n,0,0));assert(Fx3Active);
  assert(GSU.nRamBanks==2&&GSU.nRomBanks==64);
  assert(retro_get_memory_size(RETRO_MEMORY_SAVE_RAM)==(type==0x18?0x20000:0));
  assert(!memcmp(rom,Memory.ROM,n)); /* No low-bank replication over high ROM. */
  for(unsigned bank=0;bank<256;bank++){
   for(unsigned page=0;page<16;page++)assert(Memory.MemorySpeed[bank*16+page]>0);
   if((bank&0x7f)<0x40){
    assert(Memory.Map[bank*16+3]==(uint8*)MAP_NONE);
    assert(Memory.Map[bank*16+6]==(uint8*)MAP_NONE);
    assert(Memory.Map[bank*16+7]==(uint8*)MAP_PPU);
    assert(S9xGetByte((bank<<16)|0x8123)==rom[((bank&63)<<15)+0x123]);
   }
   if((bank>=0x40&&bank<=0x6f)||bank>=0xc0)
    assert(S9xGetByte((bank<<16)|0x1234)==rom[((bank&63)<<16)+0x1234]);
  }
  for(unsigned mirror=0;mirror<4;mirror++){
   unsigned addr=0x7000+mirror*0x400;
   assert(S9xGetByte(addr+0x3b)==0x52);
   S9xSetByte(0xa5,addr+0x100);assert(S9xGetByte(0x7100)==0xa5);
   assert(S9xGetByte(addr+0x300)==((addr+0x300)>>8));
   S9xSetByte(0x42,addr+0x300);assert(Memory.FillRAM[0x3300]!=0x42);
  }
  S9xSetByte(0x44,0x7060);S9xSetByte(0x77,0x3060);assert(Memory.FillRAM[0x3060]==0x44);
  S9xSetByte(0x12,0x700123);S9xSetByte(0x34,0x710123);
  assert(Memory.SRAM[0x123]==0x12&&Memory.SRAM[0x10123]==0x34);
  assert(Memory.BlockIsROM[0xf00]); /* Old F0 SRAM alias must be ROM. */
  puts("PASS FX3 CPU mappings, MMIO mirrors/holes, VCR, ROM preservation and two RAM banks");
  for(unsigned cmd=0;cmd<=6;cmd++){
   memset(Memory.SRAM,0x5a,0x20000);start(cmd,0x60,0x100);
   for(unsigned i=0;i<0x20000;i++){
    unsigned expect=0x5a;
    if(cmd>=3&&cmd<=5&&i>=0x10000){
     unsigned a=i-0x10000,tile=a/64,row=tile/20,col=tile%20,b=a%64;
     if(col<18&&row>=(cmd-3)*9&&row<(cmd-2)*9)
      expect=(b<16&&!(b&1))||(b>=48&&(b&1))?255:0;
    }
    assert(Memory.SRAM[i]==expect);
   }
  }
  start(0,0,0x8100);start(0,0,0x0100);start(0,0x6f,0x100);start(0,0x70,0x100);
  puts("PASS FX3 MERGE 0..6, clear regions/stride/planes, low-ROM mirror, extended-ROM/RAM execution, STOP R15/no IRQ");
  /* An infinite GSU branch must yield with G still set, without RON/RAN. */
  S9xResetSuperFX();Memory.ROM[0x200100]=0x05;Memory.ROM[0x200101]=0xfe;
  Memory.ROM[0x200102]=1;S9xSetByte(0x60,0x7034);S9xSetByte(0,0x701e);S9xSetByte(1,0x701f);
  assert(Memory.FillRAM[0x3030]&FLG_G);assert(S9xGetByte(0x701f)==1);
  S9xSetByte(0,0x7030);assert(!(Memory.FillRAM[0x3030]&FLG_G));
  /* Count INC R1 in a fixed branch loop at both clock selections. */
  unsigned count[2];
  for(unsigned fast=0;fast<2;fast++){
   S9xResetSuperFX();const uint8_t loop[]={0xd1,0x05,0xfd,1};
   memcpy(Memory.ROM+0x200100,loop,sizeof(loop));
   S9xSetByte(fast,0x7039);S9xSetByte(0x60,0x7034);
   S9xSetByte(0,0x701e);S9xSetByte(1,0x701f);
   assert(Memory.FillRAM[0x3030]&FLG_G);count[fast]=GSU.avReg[1];
   S9xSetByte(0,0x7030);
  }
  assert(count[0]>0&&count[1]>=2*count[0]-2&&count[1]<=2*count[0]+2);
  GSU.vRomBankReg=0;GSU.pvRomBank=GSU.apvRomBank[0];
  assert(ROM(0x8123)==ROM(0x0123));
  GSU.vRomBankReg=0x60;GSU.pvRomBank=GSU.apvRomBank[0x60];
  assert(ROM(0x1234)==Memory.ROM[0x201234]);
  printf("PASS FX3 bounded execution, CPU polling/cancellation, clock counts %u/%u, ROM data banks\n",count[0],count[1]);
  pocket_player_close();
 }
 /* No FX3 mode leakage when the next cartridge is ordinary Super FX. */
 rom[0x7fd6]=0x13;assert(!nes_init(rom,n,0,0));assert(!Fx3Active);assert(GSU.nRomBanks==32);pocket_player_close();
 free(rom);return 0;
}
