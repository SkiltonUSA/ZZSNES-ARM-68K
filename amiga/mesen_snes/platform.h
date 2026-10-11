// GPL-3.0-or-later. CPU diagnostic adapter, NOT the SNES system bus.
#pragma once
#include <stdint.h>
#include <stddef.h>
extern "C" void *memcpy(void *, const void *, size_t);
extern "C" void *memset(void *, int, size_t);
#define __forceinline __attribute__((always_inline)) inline
#define __noinline __attribute__((noinline))
#include "MemoryOperationType.h"
struct BaseState {};
enum class CpuType { Snes };
class CheatManager { public: void RefreshRamCheats(CpuType) {} };
class Emulator {
    CheatManager cheats;
public:
    template<CpuType> void ProcessInstruction() {}
    template<CpuType> void ProcessInterrupt(uint32_t, uint32_t, bool) {}
    template<CpuType> void ProcessHaltedCpu() {}
    template<CpuType> void ProcessIdleCycle() {}
    CheatManager *GetCheatManager() { return &cheats; }
};
class SnesDmaController {
public:
    bool HasPendingTransfer() { return false; }
    bool ProcessPendingTransfers() { return false; }
};
class SnesMemoryManager {
public:
    uint8_t ram[65536] = {};
    uint32_t unsupported = 0;
    uint64_t master = 0;
    uint8_t speed = 8;
    // Flat test memory in bank zero and a bank-$7e alias for long accesses.
    // Real cartridge/WRAM/IO/refresh/DMA timing is deliberately not claimed.
    bool supported(uint32_t addr) { return addr < 65536 || (addr >> 16) == 0x7e; }
    uint8_t Read(uint32_t addr, MemoryOperationType) {
        master += speed;
        if(!supported(addr)) { unsupported++; return 0; }
        return ram[addr & 65535];
    }
    void Write(uint32_t addr, uint8_t v, MemoryOperationType) {
        master += speed;
        if(!supported(addr)) { unsupported++; return; }
        ram[addr & 65535] = v;
    }
    uint16_t PeekWord(uint32_t addr) {
        if(addr >= 65535) { unsupported++; return 0; }
        return ram[addr] | (uint16_t)ram[addr+1] << 8;
    }
    void SetCpuSpeed(uint8_t s) { speed = s; }
    uint8_t GetCpuSpeed(uint32_t) { return 8; }
    void IncMasterClock4() { master += 4; }
    void IncMasterClock6() { master += 6; }
};
class SnesConsole {
public:
    Emulator emulator;
    SnesMemoryManager memory;
    SnesDmaController dma;
    Emulator *GetEmulator() { return &emulator; }
    SnesMemoryManager *GetMemoryManager() { return &memory; }
    SnesDmaController *GetDmaController() { return &dma; }
};
