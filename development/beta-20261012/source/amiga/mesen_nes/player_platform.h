// GPL-3.0-or-later. Accurate core interfaces with fixed ownership.
#pragma once
class IMemoryHandler;
class MemoryRanges {
public:
 IMemoryHandler **read,**write;
 void AddHandler(MemoryOperation op,uint16_t a,uint16_t b=0){if(!b)b=a;if(a<0x4000||b>0x4017)__builtin_trap();for(unsigned i=a;i<=b;i++){if((int)op&1)read[i-0x4000]=handler;if((int)op&2)write[i-0x4000]=handler;}}
 IMemoryHandler *handler;
};
class IMemoryHandler {public:virtual void GetMemoryRanges(MemoryRanges&)=0;virtual uint8_t ReadRAM(uint16_t)=0;virtual void WriteRAM(uint16_t,uint8_t)=0;};
class MemoryManager {
public:
 Console *console=nullptr;uint8_t ram[2048]{};
 IMemoryHandler *reads[24]{},*writes[24]{};
 uint8_t openBus=0,buttons=0,shift=0;bool strobe=false;
 uint8_t Read(uint16_t,MemoryOperationType=MemoryOperationType::Read);
 void Write(uint16_t,uint8_t,MemoryOperationType=MemoryOperationType::Write);
 uint8_t GetOpenBus(){return openBus;}
 void RegisterIODevice(IMemoryHandler *p){MemoryRanges r{reads,writes,p};p->GetMemoryRanges(r);}
};
class APU;class SoundMixer;enum class AudioChannel;
class ControlManager {public:void UpdateInputState(){}};
class Console {
public:
 MemoryManager memory{};BaseMapper *mapper=nullptr;EmulationSettings settings{};APU *apu=nullptr;SoundMixer *mixer=nullptr;ControlManager controls{};
 CPU *cpu=nullptr;PPU *ppu=nullptr;uint16_t *buffers[2],*completed=nullptr;uint32_t frames=0;
 MemoryManager *GetMemoryManager(){return &memory;}PPU *GetPpu(){return ppu;}CPU *GetCpu(){return cpu;}
 APU *GetApu(){return apu;}BaseMapper *GetMapper(){return mapper;}SoundMixer *GetSoundMixer(){return mixer;}
 EmulationSettings *GetSettings(){return &settings;}ControlManager *GetControlManager(){return &controls;}
 void InitializeRam(uint8_t *p,size_t n){memset(p,0,n);}void FrameReady(uint16_t *p){completed=p;frames++;}
 void ProcessCpuClock();void SetNextFrameOverclockStatus(bool){}
 void DebugAddTrace(const char*){}void DebugProcessInterrupt(uint16_t,uint16_t,bool){}
};
#include "APU.h"
