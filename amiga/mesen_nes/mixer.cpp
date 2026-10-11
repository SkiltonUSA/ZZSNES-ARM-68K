#include "platform.h"
#include "SoundMixer.h"
#include "mix_lut.h"
void player_blip_rate(blip_t *);
void SoundMixer::Reset(){
 memset(delta,0,sizeof(delta));memset(pending,0,sizeof(pending));memset(current,0,sizeof(current));previous=0;count=0;overflow=false;
 if(!blip){blip=blip_new(1024);if(!blip)__builtin_trap();player_blip_rate(blip);}blip_clear(blip);
}
void SoundMixer::AddDelta(AudioChannel ch,uint32_t t,int16_t v){if((unsigned)ch>=5||t>=CycleLength)__builtin_trap();delta[t][(unsigned)ch]+=v;pending[t>>5]|=1u<<(t&31);}
void SoundMixer::PlayAudioBuffer(uint32_t cycles){
 // Channels can submit events out of order. A timestamp bitmap preserves
 // chronological mixing and combines simultaneous nonlinear DAC changes.
 // Include the end timestamp, just as the dense mixer does; retain later bits.
 if(cycles>=CycleLength)__builtin_trap();
 for(unsigned word=0;word<=cycles/32;word++){
  uint32_t mask=~0u;if(word==cycles/32&&(cycles&31)!=31)mask=(1u<<((cycles&31)+1))-1;
  uint32_t bits=pending[word]&mask;pending[word]&=~mask;
  while(bits){
   unsigned t=word*32+__builtin_ctz(bits);bits&=bits-1;
   bool changed=false;for(unsigned ch=0;ch<5;ch++)if(delta[t][ch]){current[ch]+=delta[t][ch];delta[t][ch]=0;changed=true;}
   if(!changed)continue;
   unsigned sq=current[0]+current[1],tnd=3*current[2]+2*current[3]+current[4];
   // Same two nonlinear DAC formulae as upstream mono, exact integer floor.
   int out=pulse_lut[sq]+tnd_lut[tnd];
   blip_add_delta(blip,t,out-previous);previous=out;
  }
 }

 blip_end_frame(blip,cycles);int n=blip_samples_avail(blip);
 if(n<0||count+(unsigned)n>1024){overflow=true;blip_clear(blip);return;}
 count+=blip_read_samples(blip,samples+count,n,0);
}
