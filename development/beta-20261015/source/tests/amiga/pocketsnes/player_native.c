#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*);
int nes_frame(uint16_t,int(*)(void));
uint16_t *nes_pixels(void);uint32_t nes_frames(void);uint32_t nes_hash(void);
unsigned nes_audio(const int16_t**);void pocket_player_close(void);
void pocket_heap_init(void){}
static int stop(void){return 1;}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t*rom=malloc(524288);assert(rom);assert(fread(rom,1,524288,f)==524288);fclose(f);
 assert(nes_init(rom,1,0,0)==1);rom[0]^=1;assert(nes_init(rom,524288,0,0)==1);rom[0]^=1;
 assert(!nes_init(rom,524288,0,0));assert(nes_frame(0,stop)==2&&nes_frames()==0);
 uint32_t pcm=2166136261u,pairs=0,nonzero=0;
 for(unsigned n=0;n<600;n++){
  uint16_t input=((n>=360&&n<368)||(n>=480&&n<488))?8:0;
  assert(!nes_frame(input,0));const int16_t*p;unsigned count=nes_audio(&p);assert(count<=1024);pairs+=count;
  for(unsigned i=0;i<count*2;i++){pcm=(pcm^(uint16_t)p[i])*16777619u;if(p[i])nonzero++;}
  uint16_t*v=nes_pixels();uint32_t visible=2166136261u;
  for(unsigned i=0;i<256*240;i++)if(i<8*256||i>=232*256)assert(v[i]==0);else visible=(visible^v[i])*16777619u;
  if(n%60==59)printf("FRAME n=%u padded=%08x visible=%08x pcm=%08x pairs=%u nonzero=%u\n",n+1,nes_hash(),visible,pcm,pairs,nonzero);
 }
 assert(pcm==0xa2053b75&&pairs==320813&&nonzero==327934);pocket_player_close();free(rom);return 0;
}
