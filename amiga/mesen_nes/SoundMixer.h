// GPL-3.0-or-later. Five-channel mono mixer; upstream nonlinear DAC and blip_buf.
#pragma once
#include <stdint.h>
#include "blip_buf.h"
enum class AudioChannel { Square1,Square2,Triangle,Noise,DMC };
class SoundMixer {
 int16_t delta[10000][5]{};uint32_t pending[(10000+31)/32]{};
 int current[5]{};int previous=0;
 blip_t *blip=nullptr;
public:
 static constexpr uint32_t CycleLength=10000;
 int16_t samples[1024]{};unsigned count=0;bool overflow=false;
 void Reset();void SetNesModel(NesModel){}
 void AddDelta(AudioChannel channel,uint32_t time,int16_t value);
 void PlayAudioBuffer(uint32_t cycles);
};
