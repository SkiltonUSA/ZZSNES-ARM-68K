// GPL-3.0-or-later. Actual core replay plus rejection before mutation.
#include "platform.h"
#include "core.h"
#include <cassert>
#include <vector>
#include <fstream>
extern unsigned fc_state_store(uint8_t *,unsigned);
extern int fc_state_load(const uint8_t *,unsigned);
struct result {uint32_t video,audio,cycles,samples;};
static result frame(unsigned i){assert(!fc_frame(i%31==0?8:i%7==0?0x81:0));return {fc_video_hash(),fc_audio_hash(),fc_cycles(),fc_samples()};}
int main(int argc,char **argv){assert(argc==2||argc==4);std::ifstream f(argv[1],std::ios::binary);std::vector<uint8_t> rom((std::istreambuf_iterator<char>(f)),{});assert(!fc_init(rom.data(),rom.size()));
 uint8_t saved[32768],other[32768],bad[32768];
 if(argc==4){
  if(!strcmp(argv[2],"save")){for(unsigned i=0;i<600;i++)frame(i);unsigned n=fc_state_store(saved,sizeof(saved));assert(n>32);FILE *s=fopen(argv[3],"wb");assert(s&&fwrite(saved,1,n,s)==n);fclose(s);}
  else {FILE *s=fopen(argv[3],"rb");assert(s);unsigned n=fread(saved,1,sizeof(saved),s);fclose(s);assert(fc_state_load(saved,n));}
  for(unsigned i=0;i<120;i++){
   if(!strcmp(argv[2],"load-zero")){
    assert(!fc_frame(0));uint32 h=2166136261u;
    for(unsigned j=0;j<61440;j++)h=(h^((XBuf[j]&63)|((XDBuf[j]&7)<<6)))*16777619u;
    printf("FRAME n=%u cycles=%u core_hash=%08x samples=%u\n",i+1,fc_cycles(),h,fc_samples());
   }else {result r=frame(i);printf("%u %08x %08x %u %u\n",i,r.video,r.audio,r.cycles,r.samples);}
  }return 0;
 }
 for(unsigned start: {1u,60u,240u,600u}){
  for(unsigned i=0;i<start;i++)frame(i);
  unsigned n=fc_state_store(saved,sizeof(saved));assert(n>32&&n<=32768);
  assert(!fc_state_store(other,n-1));
  result expected[120];for(unsigned i=0;i<120;i++)expected[i]=frame(i);
  assert(fc_state_load(saved,n));
  for(unsigned i=0;i<120;i++){result r=frame(i);if(memcmp(&r,&expected[i],sizeof(r))){printf("FAIL frame=%u start=%u video=%08x/%08x audio=%08x/%08x cycles=%u/%u samples=%u/%u\n",i,start,r.video,expected[i].video,r.audio,expected[i].audio,r.cycles,expected[i].cycles,r.samples,expected[i].samples);return 1;}}
  unsigned m=fc_state_store(other,sizeof(other));assert(m==n);
  for(unsigned pos: {0u,4u,8u,12u,16u,20u,24u,28u,32u,n-1}){
   memcpy(bad,saved,n);bad[pos]^=0x80;assert(!fc_state_load(bad,n));uint8_t after[32768];assert(fc_state_store(after,sizeof(after))==m);assert(!memcmp(after,other,m));
  }assert(!fc_state_load(saved,n-1));printf("PASS replay start=%u 120 frames video/PCM/cycles, %u state bytes; bad states unchanged\n",start,n);
 }
}
