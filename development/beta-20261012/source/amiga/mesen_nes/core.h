#pragma once
#include <stdint.h>
// 0 = success; invalid/unsupported ROM = 1; cancelled = 2; cycle bound = 3.
int nes_init(const uint8_t *rom,uint32_t size,uint16_t *a,uint16_t *b);
int nes_frame(uint8_t buttons,int (*stop)());
uint16_t *nes_pixels();
uint32_t nes_frames();
uint32_t nes_cycles();
uint32_t nes_hash();

#ifdef NES_PLAYER
unsigned nes_audio(const int16_t **);
unsigned nes_save(uint8_t **);
#endif
