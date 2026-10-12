#include "core.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
int main(int argc,char **argv){
 if(argc!=3)return 2;FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 std::vector<uint8_t> rom(1048592);size_t n=fread(rom.data(),1,rom.size(),f);fclose(f);
 if(fc_init(rom.data(),n)){puts("INIT FAIL");return 1;}
 char *end;unsigned frames=strtoul(argv[2],&end,10);if(*end||!frames||frames>3600)return 2;uint32_t pcm=0,nz=0;
 auto t=std::chrono::steady_clock::now();
 for(unsigned i=1;i<=frames;i++){
  if(fc_frame(0)){printf("FRAME FAIL %u\n",i);return 1;}
  pcm+=fc_samples();nz+=fc_nonzero();
  printf("FRAME n=%u cycles=%u video=%08x audio=%08x samples=%u nonzero=%u\n",i,fc_cycles(),fc_video_hash(),fc_audio_hash(),fc_samples(),fc_nonzero());
 }
 auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-t).count();
 printf("END frames=%u samples=%u nonzero=%u host_ms=%g\n",frames,pcm,nz,ns/1e6);
 return 0;
}
