// GPL-3.0-or-later. The instruction implementation is Mesen's vendored CPU.
#include "platform.h"
#include "CPU.h"
#include "fixture.h"
#include "bench.h"

static uint32_t registers(const State &s) {
    return (uint32_t)s.A << 24 | (uint32_t)s.X << 16 | (uint32_t)s.Y << 8 | (s.PS | 0x20);
}
extern "C" int mesen_test(int verify, MesenResult *result, int (*stop)(void), uint32_t (*clock)(void)) {
    mesen_progress(1);
    Console console{};
    console.memory.prg = nestest_prg;
    mesen_progress(2);
    CPU cpu(&console);
    mesen_progress(3);
    cpu.Reset(false, NesModel::NTSC);
    mesen_progress(4);
    State state{};
    state.PC = 0xc000; state.SP = 0xfd; state.PS = 0x24; state.CycleCount = 7;
    cpu.SetState(state);
    *result = {};
    mesen_progress(5);
    uint32_t start = clock ? clock() : 0;
    for(uint32_t i = 0; i < TRACE_COUNT; ++i) {
        if((i & 255) == 0 && stop && stop()) return 2;
        if(verify) {
            if((i & 255)==0)mesen_progress(0x10000+i);
            cpu.GetState(state);
            const Trace &want = reference_trace[i];
            if(state.PC != want.pc || state.SP != want.sp ||
               registers(state) != want.regs || state.CycleCount != want.cycles) {
                result->mismatch = i + 1;
                result->actual_pc = state.PC; result->expected_pc = want.pc;
                result->actual_regs = registers(state); result->expected_regs = want.regs;
                return 1;
            }
            result->checked++;
        }
        cpu.Exec();
    }
    result->ticks = clock ? clock() - start : 0;
    mesen_progress(6);
    cpu.GetState(state);
    result->cycles = (uint32_t)state.CycleCount - 7;
    result->errors = console.memory.ram[2] | (uint32_t)console.memory.ram[3] << 8;
    result->unsupported = console.memory.unsupported;
    // State/RAM signature is outside the separately measured CPU loop below.
    uint32_t hash = 2166136261u;
    for(uint32_t i = 0; i < sizeof(console.memory.ram); ++i)
        hash = (hash ^ console.memory.ram[i]) * 16777619u;
    hash = (hash ^ state.PC) * 16777619u;
    hash = (hash ^ registers(state)) * 16777619u;
    hash = (hash ^ state.SP) * 16777619u;
    result->hash = hash;
    mesen_progress(7);
    return result->errors || result->unsupported ? 1 : 0;
}
