// GPL-3.0-or-later. Original 65C816 smoke program; no cartridge/BIOS data.
#include "platform.h"
#include "SnesCpu.h"
#include "bench.h"

// Enter native 16-bit mode; sum 1024..1; long store/load; stack; decimal
// addition/subtraction; return to emulation mode; store completion marker.
static const uint8_t program[] = {
    0x18, 0xfb, 0xc2,0x30, 0xa2,0x00,0x04, 0xa9,0x00,0x00,
    0x86,0x10, 0x18, 0x65,0x10, 0xca, 0xd0,0xf8,
    0x8f,0x00,0x20,0x7e, 0xa9,0x00,0x00, 0xaf,0x00,0x20,0x7e,
    0x48, 0xa9,0x00,0x00, 0x68,
    0xf8, 0x18, 0xa9,0x99,0x99, 0x69,0x01,0x00,
    0x8d,0x02,0x20, 0xe9,0x01,0x00, 0x8d,0x04,0x20,
    0xd8, 0x38, 0xfb, 0xa9,0x5a, 0x8d,0x06,0x20, 0xdb
};
static SnesConsole console;
static uint32_t mix(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }

extern "C" int mesen_test(int verify, MesenResult *r, int (*stop)(void), uint32_t (*clock)(void)) {
    *r = {};
    memset(console.memory.ram, 0, sizeof(console.memory.ram));
    console.memory.unsupported = 0; console.memory.master = 0;
    memcpy(console.memory.ram + 0x8000, program, sizeof(program));
    console.memory.ram[0xfffc] = 0; console.memory.ram[0xfffd] = 0x80;
    SnesCpu cpu(&console);
    cpu.PowerOn();
    uint32_t start = clock ? clock() : 0, count = 0;
    uint32_t sum = 0, iterations = 0;
    for(; count < 6000; count++) {
        if(!(count & 63) && stop && stop()) return 2;
        SnesCpuState &s = cpu.GetState();
        // Independent expected arithmetic at each loop boundary.
        if(verify && s.PC == 0x8012) {
            if(s.A != 0x0200 || s.X || s.EmulationMode || (s.PS & 0x30)) {
                r->mismatch=count+1; r->actual_regs=s.A; r->expected_regs=0x200; return 1;
            }
        }
        if(verify && s.PC == 0x800f) {
            sum += 1024 - iterations++;
            if(s.A != (uint16_t)sum || s.X != 1025 - iterations) {
                r->mismatch=count+1; r->actual_regs=s.A; r->expected_regs=(uint16_t)sum; return 1;
            }
        }
        cpu.Exec();
        if(verify) r->checked++;
        if(s.StopState == SnesCpuStopState::Stopped) { count++; break; }
    }
    r->ticks = clock ? clock() - start : 0;
    const SnesCpuState &s = cpu.GetState();
    r->cycles = (uint32_t)s.CycleCount;
    r->unsupported = console.memory.unsupported;
    r->actual_pc = s.PC;
    r->expected_pc = 0x8000 + sizeof(program);
    r->actual_regs = s.A;
    r->expected_regs = 0x995a;
    // 5 setup instructions + 1024 * 5 loop instructions + 19 finishing.
    r->errors = count != 5144 || s.StopState != SnesCpuStopState::Stopped ||
        s.PC != r->expected_pc || s.A != r->expected_regs || s.X ||
        !s.EmulationMode || s.SP != 0x1ff || (s.PS & 0x38) != 0x30 ||
        console.memory.ram[0x2000] != 0 || console.memory.ram[0x2001] != 2 ||
        console.memory.ram[0x2002] || console.memory.ram[0x2003] ||
        console.memory.ram[0x2004] != 0x99 || console.memory.ram[0x2005] != 0x99 ||
        console.memory.ram[0x2006] != 0x5a || (verify && iterations != 1024);
    uint32_t h = 2166136261u;
    for(uint32_t i=0;i<65536;i++) h=mix(h,console.memory.ram[i]);
    h=mix(h,s.PC); h=mix(h,s.A); h=mix(h,s.X); h=mix(h,s.Y);
    h=mix(h,s.SP); h=mix(h,s.PS); h=mix(h,s.CycleCount);
    r->hash = h;
    return r->errors || r->unsupported ? 1 : 0;
}
