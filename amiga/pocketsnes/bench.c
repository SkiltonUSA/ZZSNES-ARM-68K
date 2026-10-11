/* Original benchmark adapter. Upstream Snes9x/PocketSNES retains its license. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "libretro.h"
#include "snes9x.h"
#include "memmap.h"
#include "ppu.h"
#include "apu.h"
#include "bench.h"
#include "fixture.h"
static struct MesenResult *result;
static unsigned video_frames,audio_frames,width,height,nonzero;
static uint32_t video_hash,audio_hash;
static int check_pixels;
static const void *last_video;static size_t last_pitch;
static bool environment(unsigned cmd,void *data){
 if(cmd==RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)return *(enum retro_pixel_format*)data==RETRO_PIXEL_FORMAT_RGB565;
 if(cmd==RETRO_ENVIRONMENT_GET_OVERSCAN){*(bool*)data=false;return true;}
 return false;
}
static void video(const void*data,unsigned w,unsigned h,size_t pitch){
 video_frames++;width=w;height=h;last_video=data;last_pitch=pitch;
}
static void check_video(void){
 const void*data=last_video;unsigned w=width,h=height;size_t pitch=last_pitch;video_hash=2166136261u;
 if(!data||w!=256||h!=224){result->errors++;return;}
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
  uint16_t pixel;memcpy(&pixel,(const uint8_t*)data+y*pitch+x*2,2);
  video_hash=(video_hash^pixel)*16777619u;

  if(check_pixels&&video_frames>1){uint16_t want=((x+y)&1)?0xf800:0x07c0;if(pixel!=want)result->mismatch++;}
 }
}
static size_t audio(const int16_t*data,size_t frames){
 audio_frames+=(unsigned)frames;
 for(size_t i=0;i<frames*2;i++){audio_hash=(audio_hash^(uint16_t)data[i])*16777619u;if(data[i])nonzero++;}
 return frames;
}
static void poll(void){}
static int16_t input(unsigned p,unsigned d,unsigned i,unsigned id){return 0;}
int mesen_test(int verify,struct MesenResult*r,int(*stop)(void),uint32_t(*clock)(void)){
#ifdef POCKET_ARM
 extern void pocket_heap_init(void);pocket_heap_init();
#endif
 memset(r,0,sizeof(*r));result=r;video_frames=audio_frames=nonzero=0;audio_hash=2166136261u;check_pixels=verify;
 retro_set_environment(environment);retro_set_video_refresh(video);retro_set_audio_sample_batch(audio);retro_set_input_poll(poll);retro_set_input_state(input);
 retro_init();struct retro_game_info game={0,pocket_rom,sizeof(pocket_rom),0};
 if(!retro_load_game(&game)){r->errors=1;retro_deinit();return 3;}
 for(unsigned i=0;i<6;i++){
  if(stop&&stop()){retro_unload_game();retro_deinit();return 2;}
  uint32_t start=clock?clock():0;retro_run();if(clock)r->ticks+=clock()-start;check_video();
  if(verify)r->checked++;
 }
 r->cycles=video_frames;r->hash=video_hash;r->actual_pc=width;r->expected_pc=height;r->actual_regs=audio_hash;r->expected_regs=audio_frames;
 if(APU.OutPorts[0]!=0xaa||APU.OutPorts[1]!=0xbb||nonzero||video_frames!=6||!audio_frames)r->errors++;
 retro_unload_game();retro_deinit();return (r->errors||r->mismatch)?5:0;
}
#ifdef POCKET_NATIVE
int main(void){for(int repeat=0;repeat<3;repeat++){struct MesenResult r;int rc=mesen_test(1,&r,0,0);printf("POCKET rc=%d checked=%u mismatches=%u frames=%u video=%08x errors=%u width=%u height=%u audio=%08x samples=%u\n",rc,r.checked,r.mismatch,r.cycles,r.hash,r.errors,r.actual_pc,r.expected_pc,r.actual_regs,r.expected_regs);if(rc||r.hash!=0x67711dc5||r.actual_regs!=0x77d82d05||r.expected_regs!=3208)return 1;}return 0;}
#endif
