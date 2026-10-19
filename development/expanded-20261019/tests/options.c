#define main expanded_tests
#include "features.c"
#undef main
#include "libretro.h"
#include "cheat_codes.h"
#include "cheats.h"
extern SCheatData Cheat;
extern unsigned nes_save(uint8_t**),nes_audio(const int16_t**);
extern void ps_set_pad2(unsigned),ps_audio_feedback(unsigned);
extern int ps_cheats(const char*);
extern uint32 S9xReadJoypad(int);
int main(void){
 uint8_t*r=rom(32768,0x7fc0,0x20,2),*ram;
 assert(ps_set_options(0)&&!nes_init(r,32768,0,0));
 assert(nes_save(&ram)==32768&&ram==Memory.SRAM);ram[17]=0x72;
 uint8_t*copy=malloc(32768);memcpy(copy,ram,32768);pocket_player_close();
 assert(!nes_init(r,32768,0,0));assert(nes_save(&ram)==32768);memcpy(ram,copy,32768);assert(Memory.SRAM[17]==0x72);
 Memory.SRAMSize=255;assert(nes_save(&ram)==131072);Memory.SRAMSize=0;assert(nes_save(&ram)==0&&ram==NULL);
 Memory.SRAMSize=5;
 ps_set_pad2(1u<<8);assert(!nes_frame(1,0));assert(S9xReadJoypad(0)&0x8000);assert(S9xReadJoypad(1)&0x80);assert(!(S9xReadJoypad(0)&0x80));
 ps_set_pad2(0);assert(!nes_frame(0,0));assert(!S9xReadJoypad(1));
 Memory.RAM[0x1234]=0x12;assert(ps_cheats("7E12345A;7E12356B"));assert(Memory.RAM[0x1234]==0x5a&&Memory.RAM[0x1235]==0x6b);
 assert(!ps_cheats("7E12345A123")&&Cheat.num_cheats==2);assert(ps_cheats(""));assert(!Cheat.num_cheats&&Memory.RAM[0x1234]==0x12);
 Memory.SRAM[17]=0x41;assert(ps_cheats("7000115A"));assert(Memory.SRAM[17]==0x5a);
 assert(!ps_cheats("7000115A;7000116B")&&Memory.SRAM[17]==0x5a);assert(ps_cheats(""));assert(Memory.SRAM[17]==0x41);
 assert(ps_cheats("DF47-0915"));assert(Cheat.num_cheats==1);assert(ps_cheats(""));
 puts("PASS cartridge SRAM, fresh-core reload, 2 independent pads and bounded PAR/Game Genie cheats");
 pocket_player_close();
 unsigned settings[]={64,128|(51u<<16),256|4|(81u<<9)};
 for(unsigned v=0;v<3;v++){
  assert(ps_set_options(settings[v])&&!nes_init(r,32768,0,0));assert(Settings.Transparency==(v!=2));
  ps_audio_feedback(0);assert(!nes_frame(0,0));assert(IPPU.RenderThisFrame);
  if(v<2){unsigned skipped=0,rendered=0;
   for(unsigned i=0;i<40;i++){ps_audio_feedback(256|512|1);assert(!nes_frame(0,0));if(IPPU.RenderThisFrame)rendered++;else skipped++;const int16_t*p;assert(nes_audio(&p)>500);}
   assert(skipped&&rendered);ps_audio_feedback(256|100);assert(!nes_frame(0,0)&&IPPU.RenderThisFrame);
   ps_audio_feedback(0);assert(!nes_frame(0,0)&&IPPU.RenderThisFrame);
  }
  uint8_t*state=malloc(PS_STATE_MAX);unsigned n=fc_state_store(state,PS_STATE_MAX);assert(n&&ps_get(state,64)==settings[v]);assert(fc_state_load(state,n));free(state);pocket_player_close();
 }
 free(copy);free(r);puts("PASS automatic frameskip feedback/recovery/render limit, uninterrupted PCM generation, transparency and option state restore");
 return 0;
}
