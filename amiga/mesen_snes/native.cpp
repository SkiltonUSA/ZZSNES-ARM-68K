#include <stdio.h>
#include "bench.h"
extern "C" void mesen_progress(uint32_t) {}
static unsigned polls;
static int stop() { return ++polls == 3; }
int main() {
    MesenResult a{},b{},c{};
    int x=mesen_test(1,&a,nullptr,nullptr),y=mesen_test(0,&b,nullptr,nullptr);
    int z=mesen_test(1,&c,stop,nullptr);
    printf("MESEN_SNES_CPU status=%d/%d checked=%u mismatch=%u cycles=%u hash=%08x errors=%u unsupported=%u pc=%04x expected_pc=%04x a=%04x expected_a=%04x cancel=%d/%u\n",
        x,y,a.checked,a.mismatch,a.cycles,a.hash,a.errors,a.unsupported,a.actual_pc,a.expected_pc,a.actual_regs,a.expected_regs,z,c.checked);
    return x || y || a.hash!=b.hash || a.cycles!=b.cycles || z!=2 || c.checked!=128;
}
