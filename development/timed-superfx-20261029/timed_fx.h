#pragma once
#include <stdint.h>
/* Master-cycle timing, independent of the host CPU and SNES CPU overclock. */
struct ps_fx_timing_state {
    unsigned enabled, debt, cache, cycles, memory, cached, mult, fmult;
};
extern struct ps_fx_timing_state ps_fx_timing;
void ps_fx_reset_timing(void);
void ps_fx_set_timed(unsigned enabled);
int ps_fx_timed_active(void);
void ps_fx_select_tables(void);
void ps_fx_store_timing(uint8_t *header);
int ps_fx_valid_timing(const uint8_t *header);
void ps_fx_load_timing(const uint8_t *header);
