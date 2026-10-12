// GPL-3.0-or-later. Actual generated worker with native platform stubs.
// The ARM instruction model separately exercises real presentation stores.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "wire.h"
#include "nes_api.h"
static uint8_t memory[0x400000];
static uint16_t pixels[61440];
static int16_t pcm[2]={123,-123};
static uint32_t frame_count,clock_value,step,scenario,present_calls;
static uint64_t fake_ticks(){return ++clock_value;}
static bool test_next_command(volatile uint8_t *base);
int nes_init(const uint8_t *,uint32_t,uint16_t *,uint16_t *){return 0;}
int nes_frame(uint8_t buttons,int (*stop)()){
 assert(buttons==8);
 if(scenario==3)ad_put(memory,ZZ_STOP,1);
 if(stop())return 2;
 frame_count++;for(unsigned i=0;i<61440;i++)pixels[i]=(uint16_t)(i&511);
 return 0;
}
uint16_t *nes_pixels(){return pixels;}
uint32_t nes_frames(){return frame_count;}
uint32_t nes_cycles(){return frame_count*29780;}
uint32_t nes_hash(){return 0x12345678u+frame_count;}
unsigned nes_audio(const int16_t **p){*p=pcm;return 2;}
unsigned nes_save(uint8_t **p){*p=nullptr;return 0;}
extern "C" uint32_t nes_present(volatile uint8_t *base,uint32_t addr,uint32_t pitch,uint32_t format,uint32_t nonce){
 assert(base==memory && frame_count==1);present_calls++;
 assert(addr==0x2200000 && pitch==1280 && format==3 && nonce==0x55aa);
 return scenario==4?7:0;
}
#include "worker-under-test.cpp"
static bool test_next_command(volatile uint8_t *base){
 const uint32_t session=0x76543210u;
 if(step){
  assert(ad_get(base+MB_REPLY,0)==step);
  uint32_t token=step^session;if(step==2&&scenario==2)token^=1;
  assert(ad_get(base+MB_REPLY,68)==(token^ZZ_XOR));
  uint32_t expected=step==1?0:scenario==2?4:scenario==3?2:scenario==4?7:0;
  assert(ad_get(base+MB_REPLY,4)==expected);
 }
 if(step==2)return false;
 step++;
 volatile uint8_t *cmd=base+MB_HOST;
 ad_put(cmd,0,step);ad_put(cmd,4,step==1?NES_INIT:NES_FRAME_PRESENT);
 ad_put(cmd,8,(step^session)^((step==2&&scenario==2)?1u:0u));
 ad_put(cmd,12,step==1?16:8|((scenario==1)?256:0));
 ad_put(cmd,16,1280);ad_put(cmd,20,3);ad_put(cmd,24,0x55aa);ad_put(cmd,28,0x2200000);
 return true;
}
int main(){
 for(scenario=0;scenario<=4;scenario++){
  memset(memory,0,sizeof(memory));memset(memory+NES_OUTPUT,0xa5,NES_OUTPUT_BYTES);
  frame_count=clock_value=step=present_calls=0;ad_put(memory,ZZ_CONTROL,0x76543210u);
  zz_worker(memory);
  assert(present_calls==((scenario==2||scenario==3)?0u:1u));
  if(scenario<2){
   assert(ad_get(memory+MB_REPLY,24)==ad_get(memory+MB_REPLY,56));
   assert(ad_get(memory+MB_REPLY,52)>0 && ad_get(memory+MB_REPLY,28)>0);
   assert(ad_get(memory+MB_REPLY,40)==2);
   assert(memory[NES_AUDIO]==0 && memory[NES_AUDIO+1]==123);
   assert(memory[NES_AUDIO+2]==255 && memory[NES_AUDIO+3]==133);
   for(unsigned i=0;i<NES_PIXELS;i++){
    if(scenario==1)assert(memory[NES_OUTPUT+2*i]==((i&511)>>8)&&memory[NES_OUTPUT+2*i+1]==(i&255));
    else assert(memory[NES_OUTPUT+2*i]==0xa5 && memory[NES_OUTPUT+2*i+1]==0xa5);
   }
  }
 }
 puts("PASS actual worker combined command: frame-before-draw, audio endian, sparse/VERIFY publication, bad token, cancellation and presentation error");
}
