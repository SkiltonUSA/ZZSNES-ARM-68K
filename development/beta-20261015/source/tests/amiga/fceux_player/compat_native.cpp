#include "platform.h"
#include "core.h"
#include "cartridge.h"
#include <cassert>
#include <vector>
#include <fstream>
extern void fc_set_region(unsigned);extern unsigned fc_get_region();
extern unsigned fc_state_store(uint8*,unsigned);extern int fc_state_load(const uint8*,unsigned);
struct Frame{uint32 v,a,c,n;};
static Frame frame(unsigned i){assert(!fc_frame(i>=90&&i<100?8:0));assert(fc_samples()<=1024);return {fc_video_hash(),fc_audio_hash(),fc_cycles(),fc_samples()};}
int main(int argc,char**argv){assert(argc>=2);std::ifstream f(argv[1],std::ios::binary);std::vector<uint8>rom((std::istreambuf_iterator<char>(f)),{});unsigned pal=argc>2?atoi(argv[2]):fc_rom_pal(rom.data(),rom.size(),argv[1]);fc_set_region(pal);assert(!fc_init(rom.data(),rom.size())&&fc_get_region()==pal);
 uint64_t total=0;for(unsigned i=0;i<600;i++){auto r=frame(i);total+=r.n;if(i==9||i==599)printf("FRAME n=%u video=%08x audio=%08x cycles=%u samples=%u\n",i+1,r.v,r.a,r.c,r.n);}
 std::vector<uint8>s(122880);unsigned n=fc_state_store(s.data(),s.size());assert(n>32);Frame ref[30];for(unsigned i=0;i<30;i++)ref[i]=frame(i);assert(fc_state_load(s.data(),n));unsigned bad=0;for(unsigned i=0;i<30;i++){auto r=frame(i);if(memcmp(&r,&ref[i],sizeof(r))){printf("REPLAY FAIL %u video=%08x/%08x audio=%08x/%08x cycles=%u/%u\n",i,r.v,ref[i].v,r.a,ref[i].a,r.c,ref[i].c);bad++;}}
 s[28]^=1;assert(!fc_state_load(s.data(),n));printf("RESULT mapper=%d PAL=%u samples=%llu state=%u replay_bad=%u\n",nes_cartridge(rom.data(),rom.size()),pal,(unsigned long long)total,n,bad);return bad?2:0;
}
