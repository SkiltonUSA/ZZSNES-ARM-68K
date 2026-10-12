// Native adapter oracle; no supplied ROM bytes included in the repository.
#include "platform.h"
#include "../fceux_bench/core.h"
#include "../mesen_nes/core.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
static uint16_t pixels[61440];
static int stop_now;
static int stopped(){return stop_now;}
int main(int argc,char **argv){
 if(argc<3||argc>4)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 std::vector<uint8_t> rom(1048593);size_t n=fread(rom.data(),1,rom.size(),f);fclose(f);
 if(n>1048592||nes_init(rom.data(),n,pixels,nullptr))return 1;
 char *end;unsigned frames=strtoul(argv[2],&end,10);if(*end||!frames||frames>3600)return 2;
 stop_now=1;if(nes_frame(0,stopped)!=2||nes_frames())return 1;stop_now=0;
 for(unsigned i=1;i<=frames;i++){
  uint8_t buttons=argc==4&&i>=90&&i<100?8:0;
  if(nes_frame(buttons,stopped)||nes_frames()!=i)return 1;
  for(unsigned j=0;j<61440;j++)if(pixels[j]!=((XBuf[j]&63)|((XDBuf[j]&7)<<6)))return 1;
  const int16_t *pcm;unsigned count=nes_audio(&pcm);uint32_t h=2166136261u;
  if(count!=fc_samples())return 1;
  for(unsigned j=0;j<count;j++){uint16_t v=pcm[j];if(pcm[j]!=(int16_t)WaveFinal[j])return 1;h=(h^(v>>8))*16777619u;h=(h^(v&255))*16777619u;}
  if(h!=fc_audio_hash())return 1;
  printf("FRAME n=%u cycles=%u core_hash=%08x raw=%08x audio=%08x samples=%u buttons=%u\n",i,nes_cycles(),nes_hash(),fc_video_hash(),h,count,buttons);
 }
 uint8_t *save=(uint8_t *)1;if(nes_save(&save)||save)return 1;
 puts("PASS adapter pixels/emphasis, PCM, cancellation and no-save contract");return 0;
}
