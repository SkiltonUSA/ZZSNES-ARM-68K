// GPL-3.0-or-later. Actual Mesen CPU and PPU, narrow NROM integration.
#include "platform.h"
#include "CPU.h"
#include "PPU.h"
#include "core.h"
// Placement construction only; no heap or C++ standard-library dependency.
inline void *operator new(size_t,void *p) noexcept { return p; }
alignas(Console) static uint8_t consoleStore[sizeof(Console)];
alignas(CPU) static uint8_t cpuStore[sizeof(CPU)];
alignas(PPU) static uint8_t ppuStore[sizeof(PPU)];
static Console *c;
uint8_t MemoryManager::Read(uint16_t a,MemoryOperationType) {
 if(a<0x2000)openBus=ram[a&2047];
 else if(a<0x4000)openBus=console->ppu->ReadRAM(a);
 else if(a==0x4016){openBus=(openBus&0xe0)|(strobe?(buttons&1):(shift&1));if(!strobe)shift=(shift>>1)|0x80;}
 else if(a==0x4017)openBus=(openBus&0xe0)|1;
 else if(a==0x4015)openBus=0; // silent APU status
 else if(a>=0x8000)openBus=prg[(a-0x8000)&prgMask];
 else if(a>=0x6000)openBus=sram[a-0x6000];
 return openBus;
}
void MemoryManager::Write(uint16_t a,uint8_t v,MemoryOperationType) {
 openBus=v;
 if(a<0x2000)ram[a&2047]=v;
 else if(a<0x4000 || a==0x4014)console->ppu->WriteRAM(a,v);
 else if(a==0x4016){if(v&1 || strobe)shift=buttons;strobe=v&1;}
 else if(a>=0x6000 && a<0x8000)sram[a-0x6000]=v;
}
int nes_init(const uint8_t *rom,uint32_t size,uint16_t *a,uint16_t *b) {
 if(!rom || size<16 || rom[0]!='N' || rom[1]!='E' || rom[2]!='S' || rom[3]!=26 ||
    (rom[4]!=1 && rom[4]!=2) || rom[5]>1 || (rom[6]&0xfe) || rom[7] ||
    rom[8]>1 || rom[9] || rom[10] || rom[11] || rom[12] || rom[13] || rom[14] || rom[15] ||
    size!=16u+rom[4]*16384u+rom[5]*8192u || !a || !b || a==b)return 1;
 c=new(consoleStore) Console{};
 c->buffers[0]=a;c->buffers[1]=b;c->memory.console=c;
 c->memory.prg=rom+16;c->memory.prgMask=rom[4]*16384u-1;
 c->mapper.chrRam=!rom[5];c->mapper.vertical=rom[6]&1;
 if(rom[5])memcpy(c->mapper.chr,rom+16+rom[4]*16384u,8192);
 c->ppu=new(ppuStore) PPU(c);c->cpu=new(cpuStore) CPU(c);
 c->ppu->SetNesModel(NesModel::NTSC);c->cpu->Reset(false,NesModel::NTSC);
 return 0;
}
int nes_frame(uint8_t buttons,int (*stop)()) {
 if(!c)return 1;
 c->memory.buttons=buttons;
 uint32_t frame=c->frames;uint64_t start=c->cpu->GetCycleCount();uint32_t n=0;
 while(frame==c->frames){
  if((n++&255)==0 && stop && stop())return 2;
  c->cpu->Exec();
  if(c->cpu->GetCycleCount()-start>40000)return 3;
 }
 return 0;
}
uint16_t *nes_pixels(){return c?c->completed:nullptr;}
uint32_t nes_frames(){return c?c->frames:0;}
uint32_t nes_cycles(){return c?(uint32_t)c->cpu->GetCycleCount():0;}
uint32_t nes_hash(){uint32_t h=2166136261u;uint16_t *p=nes_pixels();if(!p)return 0;for(unsigned i=0;i<61440;i++)h=(h^p[i])*16777619u;return h;}
