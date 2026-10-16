// GPL-3.0-or-later. Upstream CPU/PPU/APU and mapper implementations, NTSC.
#include "platform.h"
#include "CPU.h"
#include "PPU.h"
#include "SoundMixer.h"
#include "NROM.h"
#include "MMC1.h"
#include "UNROM.h"
#include "CNROM.h"
#include "MMC3.h"
#include "AXROM.h"
#include "JyCompany.h"
#include "core.h"
#include "cartridge.h"
alignas(Console) static uint8_t consoleStore[sizeof(Console)];
alignas(CPU) static uint8_t cpuStore[sizeof(CPU)];
alignas(PPU) static uint8_t ppuStore[sizeof(PPU)];
alignas(APU) static uint8_t apuStore[sizeof(APU)];
alignas(SoundMixer) static uint8_t mixerStore[sizeof(SoundMixer)];
static Console *c;
void Console::ProcessCpuClock(){mapper->ProcessCpuClock();apu->ProcessCpuClock();}
uint8_t MemoryManager::Read(uint16_t a,MemoryOperationType){
 if(a<0x2000)openBus=ram[a&2047];else if(a<0x4000)openBus=console->ppu->ReadRAM(a);
 else if(a==0x4016){openBus=(openBus&0xe0)|(strobe?(buttons&1):(shift&1));if(!strobe)shift=(shift>>1)|0x80;}
 else if(a==0x4017)openBus=(openBus&0xe0); // disconnected second controller
 else if(a<0x4018&&reads[a-0x4000]){uint8_t bit5=openBus&0x20;openBus=reads[a-0x4000]->ReadRAM(a);if(a==0x4015)openBus|=bit5;}
 else if(a>=0x4020)openBus=console->mapper->ReadCPU(a,openBus);
 return openBus;
}
void MemoryManager::Write(uint16_t a,uint8_t v,MemoryOperationType){
 openBus=v;if(a<0x2000)ram[a&2047]=v;else if(a<0x4000||a==0x4014)console->ppu->WriteRAM(a,v);
 else if(a==0x4016){if(v&1||strobe)shift=buttons;strobe=v&1;}
 else if(a<0x4018&&writes[a-0x4000])writes[a-0x4000]->WriteRAM(a,v);
 else if(a>=0x4020)console->mapper->WriteCPU(a,v);
}
int nes_init(const uint8_t *rom,uint32_t size,uint16_t *a,uint16_t *b){
 int id=nes_cartridge(rom,size);if(id<0||!a||!b||a==b)return 1;
 c=nullptr;player_reset_alloc();c=new(consoleStore) Console{};c->buffers[0]=a;c->buffers[1]=b;c->memory.console=c;
 switch(id){
  case 0:c->mapper=new(player_alloc(sizeof(NROM))) NROM{};break;
  case 1:c->mapper=new(player_alloc(sizeof(MMC1))) MMC1{};break;
  case 2:c->mapper=new(player_alloc(sizeof(UNROM))) UNROM{};break;
  case 3:c->mapper=new(player_alloc(sizeof(CNROM))) CNROM(false);break;
  case 4:c->mapper=new(player_alloc(sizeof(MMC3))) MMC3{};break;
  case 90:c->mapper=new(player_alloc(sizeof(JyCompany))) JyCompany{};break;
  case 7:c->mapper=new(player_alloc(sizeof(AXROM))) AXROM{};break;
 }
 c->mapper->Setup(c,rom,id);c->mixer=new(mixerStore) SoundMixer{};
 c->ppu=new(ppuStore) PPU(c);c->cpu=new(cpuStore) CPU(c);c->apu=new(apuStore) APU(c);
 c->memory.RegisterIODevice(c->apu);c->apu->SetNesModel(NesModel::NTSC);
 c->ppu->SetNesModel(NesModel::NTSC);c->cpu->Reset(false,NesModel::NTSC);return 0;
}
int nes_frame(uint8_t buttons,int (*stop)()){
 if(!c)return 1;c->memory.buttons=buttons;c->mixer->count=0;
 uint32_t frame=c->frames;uint64_t start=c->cpu->GetCycleCount();unsigned n=0;
 while(frame==c->frames){if((n++&255)==0&&stop&&stop())return 2;c->cpu->Exec();if(c->cpu->GetCycleCount()-start>40000)return 3;}
 c->apu->EndFrame();return c->mixer->overflow?8:0;
}
uint16_t *nes_pixels(){return c?c->completed:nullptr;}uint32_t nes_frames(){return c?c->frames:0;}
uint32_t nes_cycles(){return c?(uint32_t)c->cpu->GetCycleCount():0;}
uint32_t nes_hash(){uint32_t h=2166136261u;uint16_t *p=nes_pixels();if(!p)return 0;for(unsigned i=0;i<61440;i++)h=(h^p[i])*16777619u;return h;}
unsigned nes_audio(const int16_t **p){if(!c){*p=nullptr;return 0;}*p=c->mixer->samples;return c->mixer->count;}
unsigned nes_save(uint8_t **p){if(!c){*p=nullptr;return 0;}*p=c->mapper->SaveData();return c->mapper->SaveSize();}
#ifdef NES_NATIVE_TEST
Console *nes_test_console(){return c;}
#endif
