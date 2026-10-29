/* ZZSNES experimental scanline timing. Instruction costs are adapted from
 * Snes9x fxinst/fxemu (see README.md for pin and licensing). Not cycle exact. */
#include <string.h>
#include "fxemu.h"
#include "fxinst.h"
#include "timed_fx.h"
#include "state_format.h"
#include "core_options.h"
struct ps_fx_timing_state ps_fx_timing;
extern uint32 (*ps_timed_functions[])(uint32);
extern void (*ps_timed_opcodes[])();
extern void (*ps_timed_plots[])();
void ps_fx_reset_timing(void) {
    unsigned enabled=ps_fx_timing.enabled;
    memset(&ps_fx_timing,0,sizeof(ps_fx_timing));
    ps_fx_timing.enabled=enabled;
}
int ps_fx_timed_active(void) { return ps_fx_timing.enabled && !Fx3Active; }
void ps_fx_set_timed(unsigned enabled) {
    if(ps_fx_timing.enabled != !!enabled) ps_fx_reset_timing();
    ps_fx_timing.enabled=!!enabled;
}
void ps_fx_select_tables(void) {
    int timed=ps_fx_timed_active();
    void (**plots)()=timed?ps_timed_plots:fx_apfPlotTable;
    fx_ppfFunctionTable=timed?ps_timed_functions:fx_apfFunctionTable;
    fx_ppfOpcodeTable=timed?ps_timed_opcodes:fx_apfOpcodeTable;
    fx_ppfPlotTable=plots;
}
void ps_fx_store_timing(uint8_t *p) {
    if(!ps_fx_timing.enabled) return; /* Legacy state bytes remain zero. */
    ps_put(p,72,0x46585431); /* FXT1 */
    ps_put(p,76,ps_fx_timing.debt);
    ps_put(p,80,ps_fx_timing.cache);
}
int ps_fx_valid_timing(const uint8_t *p) {
    if(ps_get(p,8)!=3 || !(ps_get(p,64)&PS_TIMED_FX)) return !ps_get(p,72);
    return ps_get(p,72)==0x46585431 && ps_get(p,76)<256;
}
void ps_fx_load_timing(const uint8_t *p) {
    ps_fx_reset_timing();
    if(ps_fx_timing.enabled) {
        ps_fx_timing.debt=ps_get(p,76);
        ps_fx_timing.cache=ps_get(p,80);
    }
}
