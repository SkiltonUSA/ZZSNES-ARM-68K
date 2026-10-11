// GPL-3.0-or-later. Deterministic headless host services for Mesen2 SNES.
#pragma once
#ifdef SS_ARM
#include "tiny.h"
#else
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <climits>
#include <cmath>
#include <cassert>
using std::unique_ptr;
using std::vector;
#endif
#include "profile.h"
#define __forceinline inline __attribute__((always_inline))
#define __noinline __attribute__((noinline))
#define LogDebug(...) ((void)0)
[[noreturn]] inline void ss_fail(){
#ifdef SS_ARM
 ss_panic();
#else
 __builtin_trap();
#endif
}
#ifdef SS_INTEGER_CLOCK
// Exact NTSC clock ratio for this bounded test, without VFP state ownership.
inline uint64_t ss_clock(uint64_t master,uint32_t rate){
    uint64_t n=master*(uint64_t)(rate*64),q=0,r=0;
    for(int i=63;i>=0;i--){r=(r<<1)|((n>>i)&1);if(r>=21477272){r-=21477272;q|=(uint64_t)1<<i;}}
    return q;
}
#define SS_CLOCK(master,ratio,rate) ss_clock(master,rate)
#define SS_FIRST_HALF(master,ratio,rate,cycle) ((master)*(uint64_t)((rate)*64)<=((cycle)+1)*21477272u)
#else
#define SS_CLOCK(master,ratio,rate) ((master)*(ratio))
#define SS_FIRST_HALF(master,ratio,rate,cycle) ((master)*(ratio)-(cycle)<=1)
#endif
#include "Shared/MemoryType.h"
#include "Shared/MemoryOperationType.h"
enum class CpuType { Snes, Spc };
enum class ConsoleRegion { Ntsc, Pal };
enum class EventType { StartFrame, EndFrame };
enum class ConsoleNotificationType { PpuFrameDone };
enum class BreakSource { SnesInvalidPpuAccess, SnesReadDuringAutoJoy };
enum class DspInterpolationType { Gauss, Cubic, Sinc, None };
struct AddressInfo { int32_t Address; MemoryType Type; };
struct SnesConfig {
    bool EnableRandomPowerOnState=false, DisableFrameSkipping=true;
    bool HideBgLayer1=false,HideBgLayer2=false,HideBgLayer3=false,HideBgLayer4=false,HideSprites=false;
    bool ForceFixedResolution=false;
    uint32_t PpuExtraScanlinesBeforeNmi=0,PpuExtraScanlinesAfterNmi=0;
    int32_t SpcClockSpeedAdjustment=0;
    DspInterpolationType InterpolationType=DspInterpolationType::Gauss;
    uint32_t ChannelVolumes[8]={100,100,100,100,100,100,100,100};
};
class EmuSettings {
    SnesConfig cfg;
public:
    SnesConfig &GetSnesConfig(){return cfg;}
    unsigned GetEmulationSpeed(){return 100;}
    uint32_t GetRandomValue(uint32_t){return 0;}
    bool GetRandomBool(){return false;}
};
class Timer { public: unsigned GetElapsedMS(){return 100;} void Reset(){} };
struct RenderedFrame {
    uint16_t *Buffer; uint32_t Width,Height,FrameNumber;
    RenderedFrame(uint16_t *b,uint32_t w,uint32_t h,double,uint32_t n):Buffer(b),Width(w),Height(h),FrameNumber(n){}
};
class VideoDecoder {
public:
    uint16_t *pixels=nullptr;
    uint32_t width=0,height=0,frames=0;
    void UpdateFrame(RenderedFrame &f,bool,bool){
        assert(f.Width<=512&&f.Height<=478);
        width=f.Width;height=f.Height;frames++;
        pixels=f.Buffer; // valid until the next frame/console destruction
    }
};
class SoundMixer {
public:
    uint32_t samples=0,hash=2166136261u,nonzero=0;
    void PlayAudioBuffer(int16_t *p,uint32_t n,uint32_t rate){
        assert(rate==32000);
        for(uint32_t i=0;i<n*2;i++){hash=(hash^(uint16_t)p[i])*16777619u;nonzero+=p[i]!=0;}
        samples+=n;
    }
};
class CheatManager { public:
    void RefreshRamCheats(CpuType){}
    template<CpuType> bool HasCheats(){return false;}
    template<CpuType> void ApplyCheat(uint32_t,uint8_t&){}
};
class DesktopFlags { public:
    bool IsRewinding(){return false;} bool IsRecording(){return false;}
    void SendNotification(ConsoleNotificationType){}
};
class Emulator {
    EmuSettings settings; CheatManager cheats; DesktopFlags desktop;
public:
    VideoDecoder video; SoundMixer audio;
    EmuSettings *GetSettings(){return &settings;}
    CheatManager *GetCheatManager(){return &cheats;}
    VideoDecoder *GetVideoDecoder(){return &video;}
    SoundMixer *GetSoundMixer(){return &audio;}
    DesktopFlags *GetRewindManager(){return &desktop;}
    DesktopFlags *GetVideoRenderer(){return &desktop;}
    DesktopFlags *GetNotificationManager(){return &desktop;}
    template<CpuType> void ProcessInstruction(){}
    template<CpuType> void ProcessInterrupt(uint32_t,uint32_t,bool){}
    template<CpuType> void ProcessHaltedCpu(){}
    template<CpuType> void ProcessIdleCycle(){}
    template<CpuType> void ProcessPpuCycle(){} // debugger hook, not PPU scheduling
    template<CpuType,int=1,MemoryAccessFlags=MemoryAccessFlags::None> void ProcessMemoryRead(uint32_t,uint8_t,MemoryOperationType){}
    template<CpuType,int=1,MemoryAccessFlags=MemoryAccessFlags::None> bool ProcessMemoryWrite(uint32_t,uint8_t,MemoryOperationType){return true;}
    template<CpuType> void ProcessPpuRead(uint32_t,uint8_t,MemoryType){}
    template<CpuType> void ProcessPpuWrite(uint32_t,uint8_t,MemoryType){}
    void ProcessEvent(EventType){} void BreakIfDebugging(CpuType,BreakSource){}
    void RegisterMemory(MemoryType,void*,uint32_t){}
    bool IsDebugging(){return false;} bool IsRunAheadFrame(){return false;}
};
// Enhancement hardware is never instantiated by the ordinary-cart adapter.
class Sa1 { public: uint8_t CpuRegisterRead(uint32_t){assert(false);return 0;} void CpuRegisterWrite(uint32_t,uint8_t){assert(false);} };
class Msu1 { public: uint8_t Read(uint32_t){assert(false);return 0;} void Write(uint32_t,uint8_t){assert(false);} };
