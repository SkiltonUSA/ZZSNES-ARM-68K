/* Original ROM and GSU programs; no commercial data. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snes9x.h"
#include "memmap.h"
#include "fxemu.h"
#include "fxinst.h"
#include "timed_fx.h"
#include "core_options.h"
#include "state_format.h"
#include "preferences.h"
extern FxRegs_s GSU;
extern int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*);
extern int ps_set_options(unsigned);
extern unsigned fc_state_store(uint8_t*,unsigned);
extern int fc_state_load(const uint8_t*,unsigned);
extern void pocket_player_close(void);
void pocket_heap_init(void){}
static void start(unsigned clock) {
    ps_fx_reset_timing();
    memset(GSU.apvRomBank[1],1,65536); /* NOP stream */
    GSU.pvRegisters[GSU_PBR]=1;
    GSU.pvRegisters[GSU_SCMR]=0x18;
    GSU.pvRegisters[GSU_CLSR]=clock;GSU.pvRegisters[GSU_CFGR]=0;
    GSU.pvRegisters[GSU_SFR]=FLG_G;
    GSU.pvRegisters[GSU_SFR+1]=0;
    GSU.pvRegisters[30]=0; GSU.pvRegisters[31]=1;
    GSU.pvRegisters[GSU_CBR]=GSU.pvRegisters[GSU_CBR+1]=0;
    GSU.bCacheActive=0; GSU.vPipe=1;
}
int main(void) {
    uint8_t *rom=calloc(1,65536), *state=malloc(PS_STATE_MAX);
    assert(rom && state);
    memset(rom+0x7fc0,' ',21);memcpy(rom+0x7fc0,"ZZSNES TIMED FX TEST",19);
    rom[0]=0x80;rom[1]=0xfe;rom[0x7fd5]=0x20;rom[0x7fd6]=0x13;
    rom[0x7fd7]=6;rom[0x7fd8]=5;rom[0x7fd9]=1;
    rom[0x7fdc]=rom[0x7fdd]=255;rom[0x7ffd]=128;
    assert(!nes_init(rom,65536,0,0));
    assert(!ps_fx_timing.enabled);start(0);FxEmulate(20);
    assert(GSU.avReg[15]==0x114); /* Legacy counts instructions unchanged. */
    struct ps_preferences pref={0}, loaded={0};ps_controls_default(&pref.controls);
    pref.core=PS_TIMED_FX|4;FILE *settings=tmpfile();assert(settings);
    assert(ps_preferences_write(settings,&pref));rewind(settings);
    assert(ps_preferences_read(settings,&loaded)&&loaded.core==pref.core);fclose(settings);
    assert(!ps_set_options(0x1000000));
    assert(ps_set_options(PS_TIMED_FX));
    start(0);FxEmulate(60);assert(GSU.avReg[15]==0x10a);
    start(1);FxEmulate(60);assert(GSU.avReg[15]==0x10c);
    start(0);for(unsigned i=0;i<60;i++)FxEmulate(1);
    assert(GSU.avReg[15]==0x10a && !ps_fx_timing.debt);
    puts("PASS legacy default, CLSR timing and slice overshoot carry");
    start(0);GSU.bCacheActive=1;GSU.vCacheBaseReg=0x100;GSU.pvRegisters[GSU_CBR+1]=1;
    FxEmulate(98);assert(GSU.avReg[15]==0x101 && ps_fx_timing.cache==1);
    FxEmulate(10);assert(GSU.avReg[15]==0x106);
    FxFlushCache();assert(!ps_fx_timing.cache);
    puts("PASS cache fill cost, hit cost and flush");
    const unsigned op[]={0x30,0x40,0x80,0x90,0x9f,0xef,0x4c};
    const unsigned cost[]={18,18,8,18,20,12,8};
    for(unsigned i=0;i<sizeof(op)/sizeof(op[0]);i++) {
        start(0);GSU.vPipe=op[i];FxEmulate(1);
        if(ps_fx_timing.cycles!=cost[i]) fprintf(stderr,"opcode %02x cycles %u expected %u\n",op[i],ps_fx_timing.cycles,cost[i]);
        assert(ps_fx_timing.cycles==cost[i]);
    }
    puts("PASS RAM load/store, multiply, ROM buffer and plot cycle costs");
    start(0);GSU.vPipe=0;FxEmulate(1364);
    assert(!(GSU.vStatusReg&FLG_G) && (GSU.vStatusReg&FLG_IRQ));
    assert(!ps_fx_timing.debt);
    start(0);GSU.pvRegisters[GSU_SFR]=0;
    S9xSetPPU(FLG_G,0x3030);assert(GSU.pvRegisters[30]==0);
    S9xSetPPU(1,0x301f);assert(GSU.pvRegisters[30]==0);
    S9xSuperFXExec();assert(GSU.avReg[15]>0x100 && ps_fx_timing.debt<6);
    puts("PASS STOP/IRQ and no extra budget from GO/R15 writes");
    start(0);Fx3Active=1;FxEmulate(20);
    assert(GSU.avReg[15]==0x114);Fx3Active=0;
    puts("PASS FX3 retains legacy instruction budgeting; settings persist");
    start(0);FxEmulate(1);assert(ps_fx_timing.debt==5);
    unsigned n=fc_state_store(state,PS_STATE_MAX);assert(n);
    unsigned pc=GSU.avReg[15];FxEmulate(100);
    assert(fc_state_load(state,n));assert(GSU.avReg[15]==pc && ps_fx_timing.debt==5);
    FxEmulate(5);assert(GSU.avReg[15]==pc);
    assert(ps_set_options(0));assert(fc_state_load(state,n));assert(ps_fx_timing.enabled);
    ps_put(state,76,0xffffffff);ps_put(state,28,ps_checksum(state,n));
    assert(!fc_state_load(state,n));assert(ps_fx_timing.enabled);
    assert(ps_set_options(0));n=fc_state_store(state,PS_STATE_MAX);assert(n);
    assert(!ps_get(state,72));assert(ps_set_options(PS_TIMED_FX));
    assert(fc_state_load(state,n));assert(!ps_fx_timing.enabled);
    puts("PASS timed state debt/mode, malformed metadata rejection and legacy restore");
    pocket_player_close();free(rom);free(state);return 0;
}
