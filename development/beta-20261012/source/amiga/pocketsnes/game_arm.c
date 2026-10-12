/* External-ROM, bounded 60-frame batches. No commercial ROM bytes here. */
#include <stdint.h>
#include <string.h>
#include "libretro.h"
#include "bench.h"
static unsigned frame,video_frames,width,height,nonzero,audio_frames;
static uint32_t video_hash,audio_hash;
static const void *last_video;static size_t last_pitch;
static int loaded;
static const uint32_t expected[10][4]={
 {0x62aa1dc5,0x7adcb44d,32081,0},{0xa84c96f0,0x71cd6257,64162,24664},
 {0xa84c96f0,0xc601352f,96243,24664},{0x62aa1dc5,0x304a5a9f,128325,24664},
 {0x265777dd,0x32759c35,160406,69502},{0xe69f3c20,0xb4bc1e1e,192487,118807},
 {0x27f8d2ae,0xf6940307,224569,167026},{0x5fb4105e,0x3401ef87,256650,216841},
 {0xedf1acde,0x5dbd901c,288731,272499},{0xedf1acde,0xa2053b75,320813,327934}};
static bool environment(unsigned cmd,void *data){
 if(cmd==RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)return *(enum retro_pixel_format*)data==RETRO_PIXEL_FORMAT_RGB565;
 if(cmd==RETRO_ENVIRONMENT_GET_OVERSCAN){*(bool*)data=false;return true;}return false;
}
static void video(const void*d,unsigned w,unsigned h,size_t p){video_frames++;width=w;height=h;last_video=d;last_pitch=p;}
static size_t audio(const int16_t*d,size_t n){audio_frames+=(unsigned)n;for(size_t i=0;i<n*2;i++){audio_hash=(audio_hash^(uint16_t)d[i])*16777619u;if(d[i])nonzero++;}return n;}
static void poll(void){}
static int16_t input(unsigned p,unsigned d,unsigned i,unsigned id){(void)i;return p==0&&d==RETRO_DEVICE_JOYPAD&&id==RETRO_DEVICE_ID_JOYPAD_START&&((frame>=360&&frame<368)||(frame>=480&&frame<488));}
int mesen_test(int verify,struct MesenResult*r,int(*stop)(void),uint32_t(*clock)(void)){
 extern void pocket_heap_init(void);extern const void *pocket_game_rom(void);
 (void)verify;pocket_heap_init();memset(r,0,sizeof(*r));
 if(!loaded){
  frame=video_frames=audio_frames=nonzero=0;audio_hash=2166136261u;
  retro_set_environment(environment);retro_set_video_refresh(video);retro_set_audio_sample_batch(audio);retro_set_input_poll(poll);retro_set_input_state(input);
  retro_init();struct retro_game_info game={0,pocket_game_rom(),524288,0};
  if(!retro_load_game(&game)){retro_deinit();return 3;}loaded=1;
 }
 for(unsigned n=0;n<60;n++,frame++){
  if(stop&&stop()){retro_unload_game();retro_deinit();loaded=0;return 2;}
  uint32_t start=clock?clock():0;retro_run();if(clock)r->ticks+=clock()-start;
  if(!last_video||width!=256||height!=224){r->errors++;return 5;}
  /* Pixel checksum outside the timed core interval, matching fixture timing. */
  video_hash=2166136261u;
  for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++){uint16_t pixel;memcpy(&pixel,(const uint8_t*)last_video+y*last_pitch+x*2,2);video_hash=(video_hash^pixel)*16777619u;}
 }
 r->checked=60;r->cycles=frame;r->hash=video_hash;r->actual_pc=width;r->expected_pc=height;r->actual_regs=audio_hash;r->expected_regs=audio_frames;
 unsigned k=frame/60-1;
 if(k>=10||video_frames!=frame||video_hash!=expected[k][0]||audio_hash!=expected[k][1]||audio_frames!=expected[k][2]||nonzero!=expected[k][3])r->errors++;
 if(frame==600||r->errors){retro_unload_game();retro_deinit();loaded=0;}
 return r->errors?5:0;
}
