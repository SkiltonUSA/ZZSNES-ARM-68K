/* PocketSNES libretro adapter for the existing owned-buffer Amiga frontend. */
#include <stdint.h>
#include <string.h>
#include "libretro.h"
static uint16_t picture[256*240],pad;
static int16_t sound[2048];
static unsigned frames,samples,failed;
static uint32_t hash;
static bool environment(unsigned cmd,void *data){
 if(cmd==RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)return *(enum retro_pixel_format*)data==RETRO_PIXEL_FORMAT_RGB565;
 if(cmd==RETRO_ENVIRONMENT_GET_OVERSCAN){*(bool*)data=false;return true;}return false;
}
static void video(const void*d,unsigned w,unsigned h,size_t pitch){
 if(!d||w!=256||h!=224||pitch<512){failed=1;return;}
 for(unsigned y=0;y<224;y++)memcpy(picture+(y+8)*256,(const uint8_t*)d+y*pitch,512);
}
static size_t audio(const int16_t*d,size_t n){
 if(n>1024-samples){failed=1;return n;}
 memcpy(sound+samples*2,d,n*4);samples+=(unsigned)n;return n;
}
static void poll(void){}
static int16_t input(unsigned p,unsigned d,unsigned i,unsigned id){(void)i;return p==0&&d==RETRO_DEVICE_JOYPAD&&id<12&&!!(pad&(1u<<id));}
int nes_init(const uint8_t *rom,uint32_t n,uint16_t*a,uint16_t*b){
 extern void pocket_heap_init(void);(void)a;(void)b;
 /* First preview deliberately accepts only the supplied, validated cartridge. */
 uint32_t h=2166136261u;if(n!=524288)return 1;
 for(unsigned i=0;i<n;i++)h=(h^rom[i])*16777619u;if(h!=0xae8466a1u)return 1;
 pocket_heap_init();frames=samples=failed=pad=0;memset(picture,0,sizeof(picture));
 retro_set_environment(environment);retro_set_video_refresh(video);retro_set_audio_sample_batch(audio);retro_set_input_poll(poll);retro_set_input_state(input);
 retro_init();struct retro_game_info game={0,rom,n,0};if(!retro_load_game(&game)){retro_deinit();return 1;}return 0;
}
int nes_frame(uint16_t buttons,int(*stop)(void)){
 if(stop&&stop())return 2;pad=buttons;samples=0;retro_run();frames++;
 hash=2166136261u;for(unsigned i=0;i<256*240;i++)hash=(hash^picture[i])*16777619u;
 return failed?3:0;
}
uint16_t *nes_pixels(void){return picture;}
uint32_t nes_frames(void){return frames;}
uint32_t nes_cycles(void){return frames;}
uint32_t nes_hash(void){return hash;}
unsigned nes_audio(const int16_t **p){*p=sound;return samples;}
/* Preview is read-only with respect to cartridge saves. SRAM support follows
 * separate round-trip tests, rather than writing unverified files. */
unsigned nes_save(uint8_t **p){*p=0;return 0;}
void pocket_player_close(void){retro_unload_game();retro_deinit();}
