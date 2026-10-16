// GPL-3.0-or-later. Six complete original-fixture frames, headless.
#include "SNES/SnesConsole.h"
#include "SNES/SnesCpu.h"
#include "SNES/SnesPpu.h"
#include "SNES/SnesMemoryManager.h"
#include "SNES/Spc.h"
#include "bench.h"
#include "fixture.h"
extern "C" void ss_heap_reset();
extern "C" uint32_t ss_heap_used();
extern "C" int mesen_test(int verify,MesenResult*r,int(*stop)(),uint32_t(*clock)()){
 #ifdef SS_PROFILE
 ss_profile_reset(clock);
#endif
 ss_heap_reset();init_rom();*r={};SnesConsole *c=new SnesConsole();
 if(!c->Load(system_rom,sizeof(system_rom))){delete c;return 3;}
 for(unsigned i=0;i<6;i++){
  #ifdef SS_PROFILE
  ss_profile.active=true;
  #endif
  unsigned start=clock?clock():0;
  int rc=c->RunFrame(stop);if(clock)r->ticks+=clock()-start;
  #ifdef SS_PROFILE
  ss_profile.active=false;
  #endif
  if(rc){delete c;return rc;}
  auto &v=c->GetEmulator()->video;
  if(v.width!=256||v.height!=239){delete c;return 4;}
  if(verify&&i){
   // Independent pixel oracle: four-colour tile fixture, red/green pixels.
   for(unsigned y=0;y<239;y++)for(unsigned x=0;x<256;x++){
    uint16_t want=(y<7||y>=231)?0:((x+y)&1)?0x03e0:0x001f;
    if(v.pixels[y*256+x]!=want){r->mismatch=i+1;delete c;return 5;}
   }
  }
  if(verify)r->checked++;
 }
 auto &e=*c->GetEmulator();auto &v=e.video;
 r->cycles=(uint32_t)c->GetCpu()->GetCycleCount();
 r->hash=2166136261u;for(unsigned i=0;i<v.width*v.height;i++)r->hash=(r->hash^v.pixels[i])*16777619u;
 r->actual_pc=(uint32_t)c->GetMasterClock();r->expected_pc=(uint32_t)c->GetSpc()->GetState().Cycle;
 r->actual_regs=e.audio.hash;r->expected_regs=e.audio.samples;
 r->errors=e.audio.nonzero||e.audio.samples!=3119||e.audio.hash!=0x2cfa6c7d||r->actual_pc!=2093734||r->expected_pc!=199650;
 delete c;return r->errors?6:0;
}
