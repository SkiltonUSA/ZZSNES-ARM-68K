// Test the generated plan used by mmu.c, including unaligned Exec allocations.
#include <cassert>
#include <cstdio>
#include "mmu_plan.h"
int main() {
    unsigned tested = 0;
    for (unsigned offset = 0; offset < 4096; offset += 64) {
        unsigned base = 0x11000000 + offset;
        for (unsigned page = (base + 0x16f000) & ~4095u;
             page < base + 0x172000; page += 4096) {
            bool audio = page < base + 0x171000 && page + 4096 > base + 0x170000;
            assert(mb_page_attr(base, page) ==
                   ((audio ? MB_PAGE_SHARED : MB_PAGE_PRIVATE) | 1));
            ++tested;
        }
        assert(mb_page_attr(base, (base + 0x200000) & ~4095u) == (MB_PAGE_PRIVATE | 1));
        assert(mb_page_attr(base, (base + 0x180000) & ~4095u) == (MB_PAGE_SHARED | 1));
        assert(mb_page_attr(base, base + MB_ALLOCATION + 4096) == 0);
    }
    std::printf("PASS %u audio/adjacent page attributes; ROM, mailbox and bounds preserved\n", tested);
}
