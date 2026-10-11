/* Original headless real-cartridge test harness. No ROM bytes are in source. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libretro.h"
static unsigned frame,video_frames,width,height,nonzero;
static size_t audio_frames;
static uint32_t video_hash,audio_hash=2166136261u;
static bool environment(unsigned cmd,void *data){
 if(cmd==RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)return *(enum retro_pixel_format*)data==RETRO_PIXEL_FORMAT_RGB565;
 if(cmd==RETRO_ENVIRONMENT_GET_OVERSCAN){*(bool*)data=false;return true;}
 return false;
}
static void video(const void*data,unsigned w,unsigned h,size_t pitch){
 if(!data||w>512||h>480){fprintf(stderr,"Invalid video\n");exit(2);}
 video_frames++;width=w;height=h;video_hash=2166136261u;
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
  uint16_t pixel;memcpy(&pixel,(const uint8_t*)data+y*pitch+x*2,2);
  video_hash=(video_hash^pixel)*16777619u;
 }
}
static size_t audio(const int16_t*data,size_t frames){
 audio_frames+=frames;
 for(size_t i=0;i<frames*2;i++){audio_hash=(audio_hash^(uint16_t)data[i])*16777619u;if(data[i])nonzero++;}
 return frames;
}
static void poll(void){}
static int16_t input(unsigned p,unsigned d,unsigned i,unsigned id){
 /* Deterministic, declared input: press Start briefly at frames 360 and 480. */
 return p==0&&d==RETRO_DEVICE_JOYPAD&&id==RETRO_DEVICE_ID_JOYPAD_START&&
        ((frame>=360&&frame<368)||(frame>=480&&frame<488));
}
int main(int argc,char**argv){
 if(argc!=2)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 if(fseek(f,0,SEEK_END))return 2;long n=ftell(f);if(n<=0||n>6*1024*1024)return 2;rewind(f);
 void*rom=malloc((size_t)n);if(!rom||fread(rom,1,n,f)!=(size_t)n)return 2;fclose(f);
 retro_set_environment(environment);retro_set_video_refresh(video);retro_set_audio_sample_batch(audio);retro_set_input_poll(poll);retro_set_input_state(input);
 retro_init();struct retro_game_info game={0,rom,(size_t)n,0};if(!retro_load_game(&game))return 3;
 for(frame=0;frame<600;frame++){
  retro_run();
  if(frame%60==59)printf("FRAME n=%u video=%08x width=%u height=%u pcm=%08x samples=%zu nonzero=%u\n",frame+1,video_hash,width,height,audio_hash,audio_frames,nonzero);
 }
 retro_unload_game();retro_deinit();free(rom);
 if(video_frames!=600||!nonzero)return 4;
 puts("PASS native game startup; active PCM generated; no display/audio playback or hardware timing");return 0;
}
