/* Original fixture: restoring a state must rebuild non-serialized sprite data. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snes9x.h"
#include "ppu.h"
#include "gfx.h"
#include "state_format.h"
int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*);
int nes_frame(uint16_t,int(*)(void));
unsigned fc_state_store(uint8_t*,unsigned);
int fc_state_load(const uint8_t*,unsigned);
uint32_t nes_hash(void);
unsigned nes_audio(const int16_t**);
void pocket_player_close(void);
void pocket_heap_init(void){}
static uint32_t audio_hash(void){
 const int16_t*p;unsigned n=nes_audio(&p);uint32_t h=n;
 for(unsigned i=0;i<n*2;i++)h=(h^(uint16_t)p[i])*16777619u;
 return h;
}
int main(int argc,char**argv){
 assert(argc==2);uint8_t rom[32768];FILE*f=fopen(argv[1],"rb");
 assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
 assert(!nes_init(rom,sizeof(rom),0,0));
 for(unsigned i=0;i<6;i++)assert(!nes_frame(0,0));
 /* A single visible large sprite; the remainder are outside the screen. */
 for(unsigned i=0;i<128;i++)PPU.OBJ[i].HPos=300;
 PPU.FirstSprite=7;PPU.OBJSizeSelect=1;
 PPU.OBJ[7].HPos=12;PPU.OBJ[7].VPos=19;PPU.OBJ[7].Size=1;
 S9xSetupOBJ();assert(!IPPU.OBJChanged);
 uint8_t*s=malloc(PS_STATE_MAX);assert(s);
 unsigned n=fc_state_store(s,PS_STATE_MAX);assert(n&&ps_valid(s,n));
 uint32_t video[30],audio[30];
 for(unsigned i=0;i<30;i++){
  assert(!nes_frame(0,0));video[i]=nes_hash();audio[i]=audio_hash();
 }
 /* A new process has no prior derived list. Also exercise a stale list from
  * a different scene, which a same-process load must replace. */
 for(unsigned fresh=0;fresh<2;fresh++){
  memset(GFX.OBJList,fresh?0:0x55,sizeof(GFX.OBJList));
  memset(GFX.Sizes,0,sizeof(GFX.Sizes));
  memset(GFX.VPositions,0,sizeof(GFX.VPositions));
  assert(fc_state_load(s,n));
  if(GFX.OBJList[0]!=7||GFX.OBJList[1]!=-1||GFX.Sizes[7]!=32||GFX.VPositions[7]!=19){
   puts("FAIL: restored state did not rebuild its sprite list");
   pocket_player_close();free(s);return 42;
  }
  for(unsigned i=0;i<30;i++){
   assert(!nes_frame(0,0));assert(nes_hash()==video[i]);assert(audio_hash()==audio[i]);
  }
 }
 pocket_player_close();free(s);
 puts("PASS: fresh/stale sprite data rebuilt; exact 30-frame video and PCM replay");
 return 0;
}
