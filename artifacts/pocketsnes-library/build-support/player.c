/* PocketSNES libretro adapter for the existing owned-buffer Amiga frontend. */
#include <stdint.h>
#include <string.h>
#include "libretro.h"
#include "cartridge.h"
#include "state_format.h"
#include <stdlib.h>
static uint16_t picture[256*240],pad;
static int16_t sound[2048];
static unsigned frames,samples,failed;
static uint32_t hash,rom_id,rom_id2,rom_bytes;
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
 if(snes_header(rom,n)<0)return 1;rom_id=snes_hash(rom,n);rom_id2=snes_hash2(rom,n);rom_bytes=n;
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

/* Only integer sample data/metadata is persisted. Runtime pointers are rebuilt. */
extern uint32_t ps_audio_phase(int load,uint32_t value);
extern void ps_mixer_state(int load,void *data);
extern unsigned ps_mixer_size(void);
static unsigned extra_size(void){return sizeof(picture)+ps_mixer_size();}
unsigned fc_state_store(uint8_t*p,unsigned cap){
 unsigned core=retro_serialize_size(),extra=extra_size(),n=PS_PAYLOAD_OFFSET+core+extra;
 if(n>cap||n>PS_STATE_MAX)return 0;memset(p,0,n);memcpy(p,"ZZSNES2",8);
 ps_put(p,8,2);ps_put(p,12,n);ps_put(p,16,rom_id);ps_put(p,20,rom_id2);ps_put(p,24,rom_bytes);
 ps_put(p,32,128);ps_put(p,36,112);ps_put(p,40,core);ps_put(p,44,extra);ps_put(p,48,frames);ps_put(p,52,ps_audio_phase(0,0));ps_put(p,56,sizeof(void*));
 for(unsigned y=0;y<112;y++)for(unsigned x=0;x<128;x++){unsigned at=PS_HEADER+2*(y*128+x);uint16_t v=picture[(y*2+8)*256+x*2];p[at]=v>>8;p[at+1]=v;}
 if(!retro_serialize(p+PS_PAYLOAD_OFFSET,core))return 0;
 memcpy(p+PS_PAYLOAD_OFFSET+core,picture,sizeof(picture));ps_mixer_state(0,p+PS_PAYLOAD_OFFSET+core+sizeof(picture));
 ps_put(p,28,ps_checksum(p,n));return n;
}
static int restore(const uint8_t*p){
 unsigned core=ps_get(p,40);if(!retro_unserialize(p+PS_PAYLOAD_OFFSET,core))return 0;
 ps_audio_phase(1,ps_get(p,52));memcpy(picture,p+PS_PAYLOAD_OFFSET+core,sizeof(picture));ps_mixer_state(1,(void*)(p+PS_PAYLOAD_OFFSET+core+sizeof(picture)));
 frames=ps_get(p,48);pad=samples=failed=0;hash=2166136261u;for(unsigned i=0;i<256*240;i++)hash=(hash^picture[i])*16777619u;return 1;
}
int fc_state_load(const uint8_t*p,unsigned n){
 if(!ps_valid(p,n)||ps_get(p,16)!=rom_id||ps_get(p,20)!=rom_id2||ps_get(p,24)!=rom_bytes||ps_get(p,56)!=sizeof(void*)||
 ps_get(p,40)!=retro_serialize_size()||ps_get(p,44)!=extra_size()||PS_PAYLOAD_OFFSET+ps_get(p,40)+ps_get(p,44)!=n)return 0;
 /* Rejected upstream snapshot must not leave partially restored emulation. */
 uint8_t*backup=malloc(PS_STATE_MAX);if(!backup)return 0;unsigned saved=fc_state_store(backup,PS_STATE_MAX);
 if(!saved){free(backup);return 0;}int ok=restore(p);if(!ok)restore(backup);free(backup);return ok;
}
