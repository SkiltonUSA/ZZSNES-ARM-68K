// GPL-3.0-or-later. Bounded NTSC NROM platform; sound deliberately absent.
#pragma once
#include <stdint.h>
#include <stddef.h>
extern "C" void *memcpy(void *, const void *, size_t);
extern "C" void *memset(void *, int, size_t);
#define __forceinline __attribute__((always_inline)) inline
#include "Types.h"
enum class NesModel { Auto, NTSC, PAL, Dendy };
enum class PpuModel { Ppu2C02, Ppu2C05A, Ppu2C05B, Ppu2C05C, Ppu2C05D, Ppu2C05E };
enum class EmulationFlags { EnableOamDecay, DisablePpu2004Reads, DisablePaletteRead,
 EnablePpu2000ScrollGlitch, ForceBackgroundFirstColumn, ForceSpritesFirstColumn,
 RemoveSpriteLimit, AdaptiveSpriteLimit, DisableOamAddrBug, EnablePpuOamRowCorruption,
 EnablePpu2006ScrollGlitch, SwapDutyCycles,SilenceTriangleHighFreq,DisableNoiseModeFlag,ReduceDmcPopping,Mmc3IrqAltBehavior };
class EmulationSettings {
public:
 bool CheckFlag(EmulationFlags) { return false; }
 int GetPpuExtraScanlinesBeforeNmi() { return 0; }
 int GetPpuExtraScanlinesAfterNmi() { return 0; }
 PpuModel GetPpuModel() { return PpuModel::Ppu2C02; }
 bool GetBackgroundEnabled() { return true; }
 bool GetSpritesEnabled() { return true; }
 int GetInputPollScanline() { return 241; }
};
class Console; class CPU; class PPU;
#ifdef NES_PLAYER
#include "player_alloc.h"
#include "player_mapper.h"
#include "player_platform.h"
#else
class BaseMapper {
public:
 uint8_t chr[8192],nt[2048];
 bool chrRam=false,vertical=false;
 uint16_t ntIndex(uint16_t a) { return ((a & 1023) | (vertical ? (a & 1024) : ((a>>1)&1024))); }
 uint8_t ReadVRAM(uint16_t a,MemoryOperationType=MemoryOperationType::PpuRenderingRead) { a&=0x3fff;return a<8192?chr[a]:nt[ntIndex(a)]; }
 void WriteVRAM(uint16_t a,uint8_t v) { a&=0x3fff;if(a<8192){if(chrRam)chr[a]=v;}else nt[ntIndex(a)]=v; }
 uint8_t DebugReadVRAM(uint16_t a) { return ReadVRAM(a); }
 int ToAbsoluteChrAddress(uint16_t a) { return a&8191; }
 void NotifyVRAMAddressChange(uint16_t) {}
};
class MemoryManager {
public:
 Console *console=nullptr;
 uint8_t ram[2048],sram[8192];
 const uint8_t *prg=nullptr; uint32_t prgMask=16383;
 uint8_t openBus=0,buttons=0,shift=0;bool strobe=false;
 uint8_t Read(uint16_t,MemoryOperationType=MemoryOperationType::Read);
 void Write(uint16_t,uint8_t,MemoryOperationType=MemoryOperationType::Write);
 uint8_t GetOpenBus() { return openBus; }
};
class APU {
public:
 uint16_t GetDmcReadAddress() { return 0; }
 void SetDmcReadBuffer(uint8_t) {}
 void SetApuStatus(bool) {}
};
class ControlManager { public: void UpdateInputState() {} };
class Console {
public:
 MemoryManager memory{}; BaseMapper mapper{}; EmulationSettings settings{};
 APU apu{}; ControlManager controls{};
 CPU *cpu=nullptr;PPU *ppu=nullptr;
 uint16_t *buffers[2],*completed=nullptr;uint32_t frames=0;
 MemoryManager *GetMemoryManager(){return &memory;}
 PPU *GetPpu(){return ppu;} CPU *GetCpu(){return cpu;}
 APU *GetApu(){return &apu;} BaseMapper *GetMapper(){return &mapper;}
 EmulationSettings *GetSettings(){return &settings;}
 ControlManager *GetControlManager(){return &controls;}
 void InitializeRam(uint8_t *p,size_t n){memset(p,0,n);}
 void FrameReady(uint16_t *p){completed=p;frames++;}
 void ProcessCpuClock() {}
 void DebugAddTrace(const char *) {}
 void DebugProcessInterrupt(uint16_t,uint16_t,bool) {}
};

#endif
