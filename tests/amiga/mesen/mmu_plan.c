#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "mmu_plan.h"
static uint32_t l1[4096],l2[MB_L2_BYTES/4];
static uint32_t lookup(uint32_t page,uint32_t l2addr) {
    uint32_t d=l1[page>>20];
    if(!d)return 0;
    assert((d&1023)==1);
    uint32_t i=((d&~1023u)-l2addr)/4+((page>>12)&255);
    assert(i<MB_L2_BYTES/4);return l2[i];
}
int main(void) {
    /* Many boundary alignments and both one-section/two-section cases. */
    for(uint32_t offset=0;offset<0x100000;offset+=64) {
        uint32_t base=0x12000000+offset,table=mb_align_up(base+MB_TABLE_BEGIN,16384),pool=table+16384;
        assert(mb_make_tables(base,l1,table,l2,pool));
        unsigned populated=0;
        for(unsigned i=0;i<4096;i++)if(l1[i])populated++;
        assert(populated==((MB_ALLOCATION>>20)+1+(offset?1:0)));
        for(uint32_t page=(base&~4095u)-4096;page<base+MB_ALLOCATION+4096;page+=4096) {
            uint32_t d=lookup(page,pool),attr=d&4095;
            if(page+4096<=base || page>=base+MB_ALLOCATION) {assert(!d);continue;}
            assert((d&~4095u)==page);
            int full=page>=base && page+4096<=base+MB_ALLOCATION;
            int public_page=page<base+MB_PUBLIC_END && page+4096>base+MB_PUBLIC_BEGIN;
            int table_page=page<base+MB_TABLE_END && page+4096>base+MB_TABLE_BEGIN;
            if(full&&!public_page&&!table_page) {
                assert(((attr>>6)&7)==4 && (attr&12)==4 && !(attr&1024));
            } else {assert(((attr>>6)&7)==1 && !(attr&12) && (attr&1024));}
            assert(((attr>>4)&3)==1 && !(attr&512));
            if(page>=base+MB_TABLE_BEGIN)assert(attr&1);
        }
        assert(lookup(MB_TIMER_PAGE,pool)==(MB_TIMER_PAGE|MB_PAGE_TIMER));
        assert(!lookup(0xf8f02000,pool)); /* shared L2 controller is UNMAPPED */
        assert(!lookup(0,pool));assert(!lookup(0x30000000,pool));
    }
    assert(!mb_make_tables(0,l1,0x40000,l2,0x44000));
    assert(!mb_make_tables(0x12000001,l1,0x12044000,l2,0x12048000));
    puts("PASS: 16384 allocation alignments; private/shared/table/boundary permissions; L2 and unrelated RAM unmapped");
}
