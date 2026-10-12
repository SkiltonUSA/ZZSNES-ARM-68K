#pragma once
#include <stdint.h>
int fc_init(const uint8_t *,uint32_t);
int fc_frame(uint8_t);
uint32_t fc_video_hash();
uint32_t fc_audio_hash();
uint32_t fc_samples();
uint32_t fc_cycles();
uint32_t fc_nonzero();
const uint8_t *fc_pixels();
