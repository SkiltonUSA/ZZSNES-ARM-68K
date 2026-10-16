// GPL-3.0-or-later. Adapt FCEUX frames to the existing ZZNes transport.
#include "platform.h"
#include "../fceux_bench/core.h"
#include "../mesen_nes/core.h"
static uint16_t *completed;
static int16_t pcm[1024];
static uint32_t frames;
int nes_init(const uint8_t *rom,uint32_t size,uint16_t *a,uint16_t *b){
 (void)b;
 if(!a)return 1;
 int error=fc_init(rom,size);if(error)return error;
 completed=a;memset(completed,0,61440*sizeof(*completed));return 0;
}
int nes_frame(uint8_t buttons,int (*stop)()){
 if(!completed)return 1;
 if(stop&&stop())return 2;
 int error=fc_frame(buttons);if(error)return error;
 if(stop&&stop())return 2;
 // FCEUX stores priority/status in high palette bits and emphasis separately.
 // The shared presenter expects 6-bit palette + 3-bit emphasis, not RGB bytes.
 for(unsigned i=0;i<61440;i++)completed[i]=(XBuf[i]&63)|((XDBuf[i]&7)<<6);
 unsigned n=fc_samples();if(n>1024)return 8;
 for(unsigned i=0;i<n;i++)pcm[i]=(int16_t)WaveFinal[i];
 frames++;return 0;
}
uint16_t *nes_pixels(){return completed;}
uint32_t nes_frames(){return frames;}
uint32_t nes_cycles(){return fc_cycles();}
uint32_t nes_hash(){uint32_t h=2166136261u;if(!completed)return 0;for(unsigned i=0;i<61440;i++)h=(h^completed[i])*16777619u;return h;}
unsigned nes_audio(const int16_t **p){*p=pcm;return fc_samples();}
unsigned nes_save(uint8_t **p){*p=nullptr;return 0;} // Supported cartridges have no battery.
