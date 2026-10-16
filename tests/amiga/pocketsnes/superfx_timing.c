/* Original fixture: verify mapper timing independently of any commercial ROM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "snes9x.h"
#include "memmap.h"
#include "cpuexec.h"
#include "apu.h"
#include "getset.h"
#include "libretro.h"
void pocket_heap_init(void) {}
static bool environment(unsigned cmd,void *data){(void)cmd;(void)data;return false;}
static void step(void){CPU.Cycles+=CPU.MemSpeed;(*ICPU.S9xOpcodes[*CPU.PC++].S9xOpcode)();}
int main(void){
 retro_set_environment(environment);retro_init();
 Memory.CalculatedSize=0x100000;
 for(unsigned seed=0;seed<2;seed++){
  for(unsigned b=0;b<MEMMAP_NUM_BLOCKS;b++)Memory.MemorySpeed[b]=seed?0x55:0;
  SuperFXROMMap();
  for(unsigned bank=0;bank<0x40;bank++)for(unsigned slot=0;slot<16;slot++){
   unsigned expected=slot==2||slot==3?ONE_CYCLE:SLOW_ONE_CYCLE;
   unsigned b=bank*16+slot;
   assert(Memory.MemorySpeed[b]==expected);
   assert(Memory.MemorySpeed[b+0x800]==expected);
  }
 }
 puts("PASS: Super FX bank timing fully initialized from zero and stale tables");
 /* CMP $2140; DEX; BNE back to DEX, native mode, 8-bit A / 16-bit X. */
 const unsigned char program[]={0xcd,0x40,0x21,0xca,0xd0,0xfd};
 memcpy(Memory.ROM+0x18000,program,sizeof(program));
 S9xSetPCBase(0x038000);ICPU.ShiftedPB=0x030000;ICPU.ShiftedDB=0;
 ICPU.S9xOpcodes=S9xOpcodesM1X0;Registers.A.W=0;Registers.X.W=3;
 APU.OutPorts[0]=0;Settings.APUEnabled=1;Settings.Shutdown=0;CPU.InDMA=0;
 CPU.Cycles=0;step();assert(CPU.Cycles==30);
 step();assert(CPU.Cycles==44&&Registers.X.W==2);
 step();assert(CPU.Cycles==66&&CPU.PC-CPU.PCBase==0x8003);
 puts("PASS: mapped CPU CMP/DEX/BNE costs 30/14/22 master clocks");
 retro_deinit();return 0;
}
