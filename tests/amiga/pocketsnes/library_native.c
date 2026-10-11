#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "state_format.h"
int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*);int nes_frame(uint16_t,int(*)(void));
uint32_t nes_hash(void);uint32_t nes_frames(void);unsigned nes_audio(const int16_t**);void pocket_player_close(void);
unsigned fc_state_store(uint8_t*,unsigned);int fc_state_load(const uint8_t*,unsigned);
void pocket_heap_init(void){}
static uint32_t ah(void){const int16_t*p;unsigned n=nes_audio(&p);uint32_t h=n;for(unsigned i=0;i<n*2;i++)h=(h^(uint16_t)p[i])*16777619u;return h;}
int main(int argc,char**argv){assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t*rom=malloc(524288);assert(fread(rom,1,524288,f)==524288);fclose(f);
 assert(!nes_init(rom,524288,0,0));for(unsigned i=0;i<400;i++)assert(!nes_frame(i>=360&&i<368?8:0,0));
 uint8_t*s=malloc(PS_STATE_MAX);unsigned n=fc_state_store(s,PS_STATE_MAX);assert(n&&ps_valid(s,n));printf("STATE bytes=%u frame=%u\n",n,nes_frames());
 uint32_t video[30],audio[30];for(unsigned i=0;i<30;i++){assert(!nes_frame(0,0));video[i]=nes_hash();audio[i]=ah();}
 assert(fc_state_load(s,n));assert(nes_frames()==400);
 for(unsigned i=0;i<30;i++){assert(!nes_frame(0,0));if(nes_hash()!=video[i]||ah()!=audio[i]){printf("REPLAY mismatch frame=%u video=%08x/%08x pcm=%08x/%08x\n",i,nes_hash(),video[i],ah(),audio[i]);return 1;}}
 s[PS_HEADER+2]^=1;assert(!fc_state_load(s,n));assert(nes_frames()==430);s[PS_HEADER+2]^=1;
 ps_put(s,16,ps_get(s,16)^1);ps_put(s,28,ps_checksum(s,n));assert(!fc_state_load(s,n));assert(nes_frames()==430);
 puts("PASS exact 30-frame video/stereo replay, thumbnail integrity and wrong-ROM rejection");
 ps_put(s,16,ps_get(s,16)^1);ps_put(s,28,ps_checksum(s,n));
 pocket_player_close();f=fopen(argv[2],"rb");assert(f);assert(fread(rom,1,32768,f)==32768);fclose(f);
 assert(!nes_init(rom,32768,0,0));assert(!fc_state_load(s,n));
 for(unsigned i=0;i<6;i++)assert(!nes_frame(0,0));
 unsigned n2=fc_state_store(s,PS_STATE_MAX);assert(n2&&ps_valid(s,n2));uint32_t reference=nes_hash();assert(!nes_frame(0,0));assert(fc_state_load(s,n2)&&nes_frames()==6&&nes_hash()==reference);
 puts("PASS original second ROM has independent identity, save and restore");
 pocket_player_close();free(s);free(rom);return 0;}
