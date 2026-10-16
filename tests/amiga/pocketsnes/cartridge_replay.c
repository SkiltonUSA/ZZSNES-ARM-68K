
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "state_format.h"
int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*);int nes_frame(uint16_t,int(*)(void));
uint32_t nes_hash(void);uint32_t nes_frames(void);unsigned nes_audio(const int16_t**);void pocket_player_close(void);
unsigned fc_state_store(uint8_t*,unsigned);int fc_state_load(const uint8_t*,unsigned);
uint16_t *nes_pixels(void);
void pocket_heap_init(void){}
static uint32_t ah(void){const int16_t*p;unsigned n=nes_audio(&p);uint32_t h=n;for(unsigned i=0;i<n*2;i++)h=(h^(uint16_t)p[i])*16777619u;return h;}
static void file_write(const char*name,const void*p,unsigned n){FILE*f=fopen(name,"wb");assert(f&&fwrite(p,1,n,f)==n);fclose(f);}
static unsigned file_read(const char*name,void*p,unsigned cap){FILE*f=fopen(name,"rb");assert(f);unsigned n=fread(p,1,cap,f);assert(!ferror(f));fclose(f);return n;}
int main(int argc,char**argv){assert(argc>=2);unsigned count=argc>2?atoi(argv[2]):600;
 FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);assert(n>0&&n<=0x400000);uint8_t*rom=malloc(n);assert(fread(rom,1,n,f)==(unsigned)n);fclose(f);
 assert(!nes_init(rom,n,0,0));unsigned changes=0,last=0;
 if(argc<4)for(unsigned i=0;i<count;i++){int e=nes_frame(i>=360&&i<368?8:0,0);if(e){printf("FRAME failure %u code=%d\n",i,e);return 2;}unsigned hash=nes_hash();changes+=last!=hash;last=hash;}
 printf("RUN frames=%u changed=%u last=%08x\n",count,changes,last);fflush(stdout);
 FILE*pic=fopen("last-frame.rgb565","wb");assert(pic);assert(fwrite(nes_pixels(),2,256*240,pic)==256*240);fclose(pic);
 uint8_t*s=malloc(PS_STATE_MAX);unsigned size;
 uint32_t video[30],audio[30];
 if(argc>=4){
  size=file_read("checkpoint.zzs",s,PS_STATE_MAX);
  assert(file_read("video.hash",video,sizeof(video))==sizeof(video));
  assert(file_read("audio.hash",audio,sizeof(audio))==sizeof(audio));
 }else{
  size=fc_state_store(s,PS_STATE_MAX);assert(size&&ps_valid(s,size));
  printf("STATE bytes=%u frame=%u\n",size,nes_frames());fflush(stdout);
  file_write("checkpoint.zzs",s,size);
  for(unsigned i=0;i<30;i++){assert(!nes_frame(0,0));video[i]=nes_hash();audio[i]=ah();}
  file_write("video.hash",video,sizeof(video));file_write("audio.hash",audio,sizeof(audio));
 }
 assert(fc_state_load(s,size));assert(nes_frames()==count);
 unsigned mismatches=0;for(unsigned i=0;i<30;i++){assert(!nes_frame(0,0));if(nes_hash()!=video[i]||ah()!=audio[i]){printf("REPLAY mismatch frame=%u video=%08x/%08x pcm=%08x/%08x\n",i,nes_hash(),video[i],ah(),audio[i]);mismatches++;}}
 printf("REPLAY mismatches=%u\n",mismatches);pocket_player_close();free(s);free(rom);return mismatches?3:0;}
