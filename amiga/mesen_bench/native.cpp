#include <stdio.h>
#include "bench.h"
extern "C" void mesen_progress(uint32_t) {}
int main() {
    MesenResult verified{}, timed{};
    int a = mesen_test(1, &verified, nullptr, nullptr), b = mesen_test(0, &timed, nullptr, nullptr);
    printf("MESEN_CPU status=%d/%d checked=%u mismatch=%u cycles=%u hash=%08x errors=%u unsupported=%u actual_pc=%04x expected_pc=%04x actual_regs=%08x expected_regs=%08x\n",
        a,b,verified.checked,verified.mismatch,verified.cycles,verified.hash,verified.errors,
        verified.unsupported,verified.actual_pc,verified.expected_pc,verified.actual_regs,verified.expected_regs);
    return a || b || verified.hash != timed.hash || verified.cycles != timed.cycles;
}
