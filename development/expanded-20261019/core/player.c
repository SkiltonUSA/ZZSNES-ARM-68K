/* PocketSNES libretro adapter for the existing owned-buffer Amiga frontend. */
#include <stdint.h>
#include <string.h>
#include "libretro.h"
#include "cartridge.h"
#include "state_format.h"
#include "region.h"
#include "core_options.h"
#include <stdlib.h>
#include <stdio.h>
static uint16_t picture[256*240],picture_full[512*480],pad,pad2;
static unsigned options,full_dirty,rendered;
unsigned ps_video_rendered(void){return rendered;}
static void (*audio_status)(bool,unsigned,bool);
void ps_set_pad2(unsigned v){pad2=v&4095;}
void ps_audio_feedback(unsigned v){if(audio_status)audio_status(!!(v&256),v&127,!!(v&512));}

int ps_set_options(unsigned v){if(!ps_options_valid(v))return 0;options=v;return 1;}
uint16_t *ps_full_pixels(void){
 if(full_dirty){for(unsigned y=0;y<480;y++)for(unsigned x=0;x<256;x++)picture_full[y*512+x*2]=picture_full[y*512+x*2+1]=picture[(y/2)*256+x];full_dirty=0;}return picture_full;
}
static int16_t sound[2048];
static unsigned frames,samples,failed,height=224;
static uint32_t hash,rom_id,rom_id2,rom_bytes;
static unsigned cart_type;
static bool environment(unsigned cmd,void *data){
 if(cmd==RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)return *(enum retro_pixel_format*)data==RETRO_PIXEL_FORMAT_RGB565;
 if(cmd==RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK){audio_status=data?((struct retro_audio_buffer_status_callback*)data)->callback:0;return true;}
 if(cmd==RETRO_ENVIRONMENT_GET_VARIABLE){
 struct retro_variable *v=data;static const char *interval[]={"0","1","2","3"};
 static char strength[4],threshold[4];
 if(!strcmp(v->key,"snes9x2002_frameskip"))v->value=((options>>6)&3)==1?"auto":((options>>6)&3)==2?"auto_threshold":(options&3)?"fixed_interval":"disabled";
 else if(!strcmp(v->key,"snes9x2002_transparency"))v->value=(options&PS_NO_TRANSPARENCY)?"disabled":"enabled";
 else if(!strcmp(v->key,"snes9x2002_low_pass_range")){snprintf(strength,sizeof(strength),"%u",ps_filter_strength(options));v->value=strength;}
 else if(!strcmp(v->key,"snes9x2002_frameskip_threshold")){snprintf(threshold,sizeof(threshold),"%u",ps_skip_threshold(options));v->value=threshold;}
 else if(!strcmp(v->key,"snes9x2002_frameskip_interval"))v->value=interval[options&3];
 else if(!strcmp(v->key,"snes9x2002_low_pass_filter"))v->value=(options&4)?"enabled":"disabled";
 else if(!strcmp(v->key,"snes9x2002_overclock_cycles"))v->value=((options>>3)&3)==2?"max":((options>>3)&3)==1?"compatible":"disabled";
 else return false;return true;}
 if(cmd==RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE){*(bool*)data=false;return true;}
 if(cmd==RETRO_ENVIRONMENT_GET_OVERSCAN){*(bool*)data=false;return true;}return false;
}
static void video(const void*d,unsigned w,unsigned h,size_t pitch){
 /* libretro NULL means duplicate the previous picture (intentional frameskip). */
 if(!d)return;rendered=1;
 if((w!=256&&w!=512)||(h!=224&&h!=239&&h!=448&&h!=478)||pitch<w*2){failed=1;return;}
 if(w==256&&h<=240&&!(options&32)){
  if(height!=h)memset(picture,0,sizeof(picture));height=h;
  for(unsigned y=0;y<h;y++)memcpy(picture+(y+(h==224?8:0))*256,(const uint8_t*)d+y*pitch,512);
  full_dirty=1;return;
 }
 full_dirty=0;height=h>240?h/2:h;
 unsigned top=height==224?16:0,ys=h>240?1:2,xs=w==512?1:2;
 memset(picture_full,0,sizeof(picture_full));
 for(unsigned y=0;y<h;y++){
  const uint16_t *row=(const uint16_t*)((const uint8_t*)d+y*pitch);
  uint16_t *out=picture_full+(top+y*ys)*512;
  for(unsigned x=0;x<w;x++){out[x*xs]=row[x];if(xs==2)out[x*xs+1]=row[x];}
  if(ys==2)memcpy(out+512,out,1024);
 }
 /* Low-resolution presentation remains available; full detail is retained. */
 for(unsigned y=0;y<240;y++)for(unsigned x=0;x<256;x++)picture[y*256+x]=picture_full[y*1024+x*2];
}
static size_t audio(const int16_t*d,size_t n){
 if(n>1024-samples){failed=1;return n;}
 memcpy(sound+samples*2,d,n*4);samples+=(unsigned)n;return n;
}
static void poll(void){}
static int16_t input(unsigned p,unsigned d,unsigned i,unsigned id){(void)i;return p<2&&d==RETRO_DEVICE_JOYPAD&&id<12&&!!((p?pad2:pad)&(1u<<id));}
int nes_init(const uint8_t *rom,uint32_t n,uint16_t*a,uint16_t*b){
 extern void pocket_heap_init(void);(void)a;(void)b;
 /* Reject unsupported cartridge hardware before initializing the core. */
 if(snes_header(rom,n)<0)return 1;rom_id=snes_hash(rom,n);rom_id2=snes_hash2(rom,n);rom_bytes=n;cart_type=rom[snes_header(rom,n)+22];
 pocket_heap_init();frames=samples=failed=pad=pad2=full_dirty=0;audio_status=0;height=224;memset(picture,0,sizeof(picture));memset(picture_full,0,sizeof(picture_full));
 retro_set_environment(environment);retro_set_video_refresh(video);retro_set_audio_sample_batch(audio);retro_set_input_poll(poll);retro_set_input_state(input);
 retro_init();struct retro_game_info game={0,rom,n,0};if(!retro_load_game(&game)){retro_deinit();return 1;}return 0;
}
int nes_frame(uint16_t buttons,int(*stop)(void)){
 if(stop&&stop())return 2;pad=buttons;samples=rendered=0;retro_run();frames++;
 hash=2166136261u;for(unsigned i=0;i<256*240;i++)hash=(hash^picture[i])*16777619u;
 return failed?3:0;
}
unsigned ps_video_height(void){return height;}
unsigned ps_frame_us(void){return ps_period_us(retro_get_region()==RETRO_REGION_PAL);}
uint16_t *nes_pixels(void){return picture;}
uint32_t nes_frames(void){return frames;}
uint32_t nes_cycles(void){return frames;}
uint32_t nes_hash(void){return hash;}
unsigned nes_audio(const int16_t **p){*p=sound;return samples;}
/* Use the core's cartridge-specific size, including special-chip SRAM. */
unsigned nes_save(uint8_t **p){unsigned n=retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);*p=n?retro_get_memory_data(RETRO_MEMORY_SAVE_RAM):0;return n;}

void pocket_player_close(void){retro_unload_game();retro_deinit();}

/* Only integer sample data/metadata is persisted. Runtime pointers are rebuilt. */
extern uint32_t ps_audio_phase(int load,uint32_t value);
extern void ps_mixer_state(int load,void *data);
extern unsigned ps_mixer_size(void);
extern unsigned ps_chip_state(int,unsigned char *);
static unsigned extra_size(void){return sizeof(picture)+sizeof(picture_full)+ps_mixer_size()+ps_chip_state(0,0);}
unsigned fc_state_store(uint8_t*p,unsigned cap){
 unsigned core=retro_serialize_size(),extra=extra_size(),n=PS_PAYLOAD_OFFSET+core+extra;
 if(n>cap||n>PS_STATE_MAX)return 0;memset(p,0,n);memcpy(p,"ZZSNES2",8);
 ps_put(p,8,3);ps_put(p,12,n);ps_put(p,16,rom_id);ps_put(p,20,rom_id2);ps_put(p,24,rom_bytes);
 ps_put(p,32,128);ps_put(p,36,112);ps_put(p,40,core);ps_put(p,44,extra);ps_put(p,48,frames);ps_put(p,52,ps_audio_phase(0,0));ps_put(p,56,sizeof(void*));ps_put(p,60,height);ps_put(p,64,options);ps_put(p,68,ps_chip_state(0,0));
 for(unsigned y=0;y<112;y++)for(unsigned x=0;x<128;x++){unsigned at=PS_HEADER+2*(y*128+x);uint16_t v=picture[((y*height/112)+(height==224?8:0))*256+x*2];p[at]=v>>8;p[at+1]=v;}
 if(!retro_serialize(p+PS_PAYLOAD_OFFSET,core))return 0;
 memcpy(p+PS_PAYLOAD_OFFSET+core,picture,sizeof(picture));ps_mixer_state(0,p+PS_PAYLOAD_OFFSET+core+sizeof(picture));
 unsigned off=PS_PAYLOAD_OFFSET+core+sizeof(picture)+ps_mixer_size();
 memcpy(p+off,ps_full_pixels(),sizeof(picture_full));off+=sizeof(picture_full);
 if(!ps_chip_state(0,p+off))return 0;
 ps_put(p,28,ps_checksum(p,n));return n;
}
static int restore(const uint8_t*p){
 extern void ps_core_refresh_options(void);
 unsigned saved_options=ps_get(p,8)==3?ps_get(p,64):0;
 unsigned next_options=(options&32)|(saved_options&~32u);if(next_options!=options){options=next_options;ps_core_refresh_options();}
 full_dirty=0;unsigned core=ps_get(p,40);height=ps_picture_height(ps_get(p,60));if(!retro_unserialize(p+PS_PAYLOAD_OFFSET,core))return 0;
 ps_audio_phase(1,ps_get(p,52));memcpy(picture,p+PS_PAYLOAD_OFFSET+core,sizeof(picture));ps_mixer_state(1,(void*)(p+PS_PAYLOAD_OFFSET+core+sizeof(picture)));
 unsigned off=PS_PAYLOAD_OFFSET+core+sizeof(picture)+ps_mixer_size();
 if(ps_get(p,8)==3){memcpy(picture_full,p+off,sizeof(picture_full));off+=sizeof(picture_full);if(!ps_chip_state(1,(unsigned char*)p+off))return 0;}
 else {extern void ps_legacy_picture(const uint16_t*,unsigned);for(unsigned y=0;y<480;y++)for(unsigned x=0;x<512;x++)picture_full[y*512+x]=picture[(y/2)*256+x/2];ps_legacy_picture(picture,height);}
 frames=ps_get(p,48);pad=pad2=samples=failed=0;hash=2166136261u;for(unsigned i=0;i<256*240;i++)hash=(hash^picture[i])*16777619u;return 1;
}
int fc_state_load(const uint8_t*p,unsigned n){
 if(!ps_valid(p,n))return 0;
 unsigned old=ps_get(p,8)==2,expected=old?sizeof(picture)+ps_mixer_size():extra_size();
 if((old?cart_type>2:(!ps_options_valid(ps_get(p,64))||ps_get(p,68)!=ps_chip_state(0,0)))||!ps_height_valid(ps_picture_height(ps_get(p,60)))||ps_get(p,16)!=rom_id||ps_get(p,20)!=rom_id2||ps_get(p,24)!=rom_bytes||ps_get(p,56)!=sizeof(void*)||
 ps_get(p,40)!=retro_serialize_size()||ps_get(p,44)!=expected||PS_PAYLOAD_OFFSET+ps_get(p,40)+ps_get(p,44)!=n)return 0;
 /* Rejected upstream snapshot must not leave partially restored emulation. */
 uint8_t*backup=malloc(PS_STATE_MAX);if(!backup)return 0;unsigned saved=fc_state_store(backup,PS_STATE_MAX);
 if(!saved){free(backup);return 0;}int ok=restore(p);if(!ok)restore(backup);free(backup);return ok;
}
