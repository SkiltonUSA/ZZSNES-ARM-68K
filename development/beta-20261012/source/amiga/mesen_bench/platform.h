// GPL-3.0-or-later. Minimal CPU-only integration; deliberately no PPU/APU.
#pragma once
#include <stdint.h>
#include <stddef.h>
extern "C" void *memcpy(void *, const void *, size_t);
extern "C" void *memset(void *, int, size_t);
#define __forceinline __attribute__((always_inline)) inline
#include "Types.h"
enum class NesModel { NTSC, PAL, Dendy };
class MemoryManager {
public:
    uint8_t ram[2048];
    const uint8_t *prg;
    uint8_t openBus = 0;
    uint32_t unsupported = 0;
    uint8_t Read(uint16_t addr, MemoryOperationType = MemoryOperationType::Read) {
        if(addr < 0x2000) openBus = ram[addr & 2047];
        else if(addr >= 0x8000) openBus = prg[addr & 0x3fff];
        else unsupported++;
        return openBus;
    }
    void Write(uint16_t addr, uint8_t value, MemoryOperationType = MemoryOperationType::Write) {
        openBus = value;
        if(addr < 0x2000) ram[addr & 2047] = value;
        // nestest automation writes sound registers; audio is intentionally absent.
        else if(addr < 0x4000 || addr > 0x4017) unsupported++;
    }
};
class PPU { public: void Run(uint64_t) {} };
class APU {
public:
    uint16_t GetDmcReadAddress() { return 0; }
    void SetDmcReadBuffer(uint8_t) {}
};
class Console {
public:
    MemoryManager memory;
    PPU ppu;
    APU apu;
    MemoryManager *GetMemoryManager() { return &memory; }
    PPU *GetPpu() { return &ppu; }
    APU *GetApu() { return &apu; }
    void ProcessCpuClock() {}
    void DebugAddTrace(const char *) {}
    void DebugProcessInterrupt(uint16_t, uint16_t, bool) {}
};
