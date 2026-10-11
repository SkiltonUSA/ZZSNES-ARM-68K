#pragma once
#include "pch.h"
class SnesCpu;class SnesPpu;class Spc;class SnesMemoryManager;class InternalRegisters;
class SnesDmaController;class SnesControlManager;class BaseCartridge;
class SnesConsole {
    Emulator emu;
    unique_ptr<SnesMemoryManager> memory;
    unique_ptr<InternalRegisters> regs;
    unique_ptr<SnesPpu> ppu;
    unique_ptr<SnesControlManager> controls;
    unique_ptr<SnesDmaController> dma;
    unique_ptr<Spc> spc;
    unique_ptr<SnesCpu> cpu;
    unique_ptr<BaseCartridge> cart;
    bool running=false;
public:
    SnesConsole();~SnesConsole();
    bool Load(const uint8_t*,uint32_t);
    int RunFrame(int(*stop)()=nullptr);
    void ProcessEndOfFrame();
    void InitializeRam(void*p,uint32_t n){memset(p,0,n);}
    Emulator *GetEmulator(){return &emu;}
    SnesMemoryManager *GetMemoryManager(){return memory.get();}
    InternalRegisters *GetInternalRegisters(){return regs.get();}
    SnesPpu *GetPpu(){return ppu.get();}
    SnesControlManager *GetControlManager(){return controls.get();}
    SnesDmaController *GetDmaController(){return dma.get();}
    Spc *GetSpc(){return spc.get();}
    SnesCpu *GetCpu(){return cpu.get();}
    BaseCartridge *GetCartridge(){return cart.get();}
    Msu1 *GetMsu1(){return nullptr;}
    ConsoleRegion GetRegion(){return ConsoleRegion::Ntsc;}
    uint64_t GetMasterClock();
    uint32_t GetMasterClockRate(){return 21477272;}
};
