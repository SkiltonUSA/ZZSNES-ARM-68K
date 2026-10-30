#include <assert.h>
#include <stdio.h>
#include <stdint.h>
typedef uint32_t ULONG;
static unsigned audio_state_drained,audio_state_drain,audio_opened,audio_error,audio_done;
static unsigned ticks,waiting,forced,stop,finish;
static ULONG us(void){return ticks;}
static void player_audio_flush(void){assert(audio_state_drain);forced++;}
static int pause_us(unsigned n){assert(audio_state_drain&&n==1000);ticks+=n;waiting++;if(finish&&waiting==finish)audio_state_drained=1;return !stop;}
#include "capture_audio.h"
static void reset(void){audio_state_drained=audio_state_drain=audio_error=audio_done=ticks=waiting=forced=stop=finish=0;audio_opened=1;}
int main(void){
 reset();finish=7;assert(fc_audio_drain()&&waiting==7&&forced==1&&!audio_state_drain);
 reset();audio_opened=0;assert(fc_audio_drain()&&!waiting&&!audio_state_drain);
 reset();audio_error=1;assert(!fc_audio_drain()&&!waiting&&!audio_state_drain);
 reset();audio_done=1;assert(!fc_audio_drain()&&!waiting&&!audio_state_drain);
 reset();stop=1;assert(!fc_audio_drain()&&waiting==1&&!audio_state_drain);
 reset();assert(!fc_audio_drain()&&ticks>1000000&&!audio_state_drain);
 puts("PASS acknowledged audio drain: completion, disabled audio, device failure, stopped worker, cancellation and bounded timeout");
}
