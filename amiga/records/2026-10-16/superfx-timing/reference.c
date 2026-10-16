#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "libretro.h"
static unsigned frame,hash,changes,last;
static bool env(unsigned c,void*d){if(c==RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)return true; if(c==RETRO_ENVIRONMENT_GET_OVERSCAN){*(bool*)d=false;return true;}return false;}
static void video(const void*d,unsigned w,unsigned h,size_t pitch){if(!d)return;hash=2166136261u;for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++)hash=(hash^((const uint16_t*)((const char*)d+y*pitch))[x])*16777619u;changes+=hash!=last;last=hash;}
static size_t audio(const int16_t*d,size_t n){return n;}
static void sample(int16_t l,int16_t r){}
static void poll(void){}
static int16_t input(unsigned p,unsigned d,unsigned i,unsigned id){return p==0&&id==3&&frame>=360&&frame<368;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);void*b=malloc(n);assert(fread(b,1,n,f)==n);fclose(f);retro_set_environment(env);retro_set_video_refresh(video);retro_set_audio_sample_batch(audio);retro_set_audio_sample(sample);retro_set_input_poll(poll);retro_set_input_state(input);retro_init();struct retro_game_info g={argv[1],b,n,0};assert(retro_load_game(&g));for(frame=0;frame<600;frame++){retro_run();if(frame<30||frame%60==59)printf("f=%u video=%08x changes=%u\n",frame+1,hash,changes);}retro_unload_game();retro_deinit();free(b);}
