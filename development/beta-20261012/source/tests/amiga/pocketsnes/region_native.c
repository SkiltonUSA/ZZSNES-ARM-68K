/* Exercise user-supplied ROMs without distributing their data. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "libretro.h"
#include "cartridge.h"
#include "region.h"
#include "state_format.h"
int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*);int nes_frame(uint16_t,int(*)(void));
uint32_t nes_hash(void);uint32_t nes_frames(void);unsigned nes_audio(const int16_t**);void pocket_player_close(void);
unsigned ps_video_height(void),ps_frame_us(void),fc_state_store(uint8_t*,unsigned);int fc_state_load(const uint8_t*,unsigned);
void pocket_heap_init(void){}
static uint32_t ah(void){const int16_t*p;unsigned n=nes_audio(&p);assert(n<=1024);uint32_t h=n;for(unsigned i=0;i<n*2;i++)h=(h^(uint16_t)p[i])*16777619u;return h;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);unsigned size=ftell(f);rewind(f);uint8_t*rom=malloc(size);assert(fread(rom,1,size,f)==size);fclose(f);unsigned skip=snes_skip(size);size-=skip;memmove(rom,rom+skip,size);
 int h=snes_header(rom,size);assert(h>=0);unsigned pal=snes_pal(rom,size);assert(!nes_init(rom,size,0,0));assert(retro_get_region()==pal);assert(ps_frame_us()==ps_period_us(pal));struct retro_system_av_info av;retro_get_system_av_info(&av);assert(av.timing.sample_rate==32040);assert(ps_frame_us()>=1000000.0/av.timing.fps&&ps_frame_us()-1000000.0/av.timing.fps<1);
 uint64_t total=0;unsigned nonzero=0,heights=0;
 for(unsigned i=0;i<600;i++){assert(!nes_frame(i>=360&&i<368?8:0,0));const int16_t*p;unsigned n=nes_audio(&p);assert(n<=1024);total+=n;for(unsigned j=0;j<n*2;j++)if(p[j])nonzero++;assert(ps_height_valid(ps_video_height()));heights|=ps_video_height()==239?2:1;}
 double expected=600.0*av.timing.sample_rate/av.timing.fps;assert(total>expected-2&&total<expected+2);
 uint8_t*state=malloc(PS_STATE_MAX);unsigned n=fc_state_store(state,PS_STATE_MAX);assert(n&&ps_valid(state,n));uint32_t hashes[20],audio[20];unsigned bad=0;unsigned geometry[20];
 for(unsigned i=0;i<20;i++){assert(!nes_frame(0,0));hashes[i]=nes_hash();audio[i]=ah();geometry[i]=ps_video_height();}
 assert(fc_state_load(state,n));assert(nes_frames()==600);
 for(unsigned i=0;i<20;i++){assert(!nes_frame(0,0));if(nes_hash()!=hashes[i]||ah()!=audio[i]||ps_video_height()!=geometry[i]){printf("REPLAY FAIL i=%u video=%08x/%08x audio=%08x/%08x height=%u/%u\n",i,nes_hash(),hashes[i],ah(),audio[i],ps_video_height(),geometry[i]);bad++;}}
 if(bad)printf("REPLAY mismatches=%u/20\n",bad);
 ps_put(state,60,240);ps_put(state,28,ps_checksum(state,n));assert(!fc_state_load(state,n));
 printf("PASS %s region=%s core_fps=%.6f period_us=%u stereo_pairs=%llu nonzero_samples=%u heights=%u frame_hash=%08x save_replay=%s\n",argv[1],pal?"PAL":"NTSC",av.timing.fps,ps_frame_us(),(unsigned long long)total,nonzero,heights,nes_hash(),bad?"FAILED":"20/20");
 pocket_player_close();free(state);free(rom);return bad?2:0;}
