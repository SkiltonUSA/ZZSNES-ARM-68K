/* Original synthetic cartridges and register operations; no commercial ROMs. */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "snes9x.h"
#include "memmap.h"
#include "gfx.h"
#include "sdd1.h"
#include "sdd1emu.h"
#include "dma.h"
#include "dsp1.h"
#include "sa1.h"
#include "fxemu.h"
#include "fxinst.h"
extern FxRegs_s GSU;
#include "cartridge.h"
#include "state_format.h"
#include "core_options.h"
extern int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*),nes_frame(uint16_t,int(*)(void));
extern void pocket_player_close(void);
extern unsigned fc_state_store(uint8_t*,unsigned);extern int fc_state_load(const uint8_t*,unsigned);
extern uint16_t *ps_full_pixels(void),*nes_pixels(void);extern int ps_set_options(unsigned);
extern void DrawHiResTile16(uint32,int32,uint32,uint32);
extern SBG BG;
extern bool8 S9xDeinitUpdate(int,int);
void pocket_heap_init(void){}
static uint8_t *rom(unsigned n,unsigned h,unsigned map,unsigned type){
 uint8_t*p=calloc(1,n);assert(p);memset(p+h,' ',21);memcpy(p+h,"ZZSNES FEATURE TEST",19);
 p[h+21]=map;p[h+22]=type;p[h+23]=n>0x400000?13:5;p[h+24]=5;p[h+25]=1;
 p[h+28]=p[h+29]=255;p[h+61]=128;unsigned code=h==0x40ffc0?0x408000:h==0xffc0?0x8000:0;
 p[code]=0x80;p[code+1]=0xfe;return p;
}
int main(void){
 unsigned maps[]={0x20,0x23,0x20,0x20,0x32,0x25,0x20,0x20,0x20},types[]={0,0x35,3,0xf3,0x43,0,0x13,0x17,0x18};
 for(unsigned t=0;t<9;t++){
  unsigned n=(t==4||t==5)?0x600000:t>=7?0x400000:0x10000,h=t==5?0x40ffc0:0x7fc0;uint8_t*p=rom(n,h,maps[t],types[t]);
  if(t>=7){const uint8_t loop[]={0xd1,0x05,0xfd,1};memcpy(p+0x100,loop,sizeof(loop));}
  assert(snes_header(p,n)==(int)h);assert(ps_set_options(0));assert(!nes_init(p,n,0,0));
  assert(Settings.SupportHiRes);assert(Memory.CalculatedSize==n);
  if(t==1)assert(Settings.SA1);
  if(t==2){assert(Settings.DSP1Master);DSP1SetByte(0,0x8000);DSP1SetByte(0,0x8000);DSP1SetByte(0x40,0x8000);}
  if(t==3){assert(Settings.C4);assert(Memory.C4RAM>=Memory.ROM+n);Memory.C4RAM[8191]=123;}
  if(t==4){assert(Settings.SDD1&&Settings.SDD1Pack);for(unsigned bank=0;bank<8;bank++){S9xSetSDD1MemoryMap(0,bank);assert(Memory.Map[0xc00]>=Memory.ROM&&Memory.Map[0xc00]+65536<=Memory.ROM+n);if(bank>=6)assert(Memory.Map[0xc00]==Memory.ROM+((bank-2)<<20));}S9xResetSDD1();
   uint8 expected[64];memset(Memory.ROM+0x1000,0,256);SDD1_decompress(expected,Memory.ROM+0x1000,64);
   memset(&DMA[0],0,sizeof(DMA[0]));DMA[0].ABank=0xc0;DMA[0].AAddress=0x1000;DMA[0].AAddressFixed=1;DMA[0].BAddress=0x18;DMA[0].TransferMode=1;DMA[0].TransferBytes=64;
   S9xSetPPU(0x80,0x2115);S9xSetPPU(0,0x2116);S9xSetPPU(0,0x2117);Memory.FillRAM[0x4801]=1;
   S9xDoDMA(0);assert(!memcmp(Memory.VRAM,expected,64));assert(!Memory.FillRAM[0x4801]);
  }
  if(t==6){
   assert(Settings.SuperFX);S9xSetPPU(0x18,0x303a);
   S9xSetPPU(8,0x3038);FxEmulate(0);assert(GSU.apvScreen[0]==GSU.pvRam+8192);
   S9xSetPPU(16,0x3038);FxEmulate(0);assert(GSU.apvScreen[0]==GSU.pvRam+16384);
   puts("PASS Super FX SCBR swap refreshes cached column pointers");
  }
  if(t==5){assert(Memory.HiROM);assert(Memory.Map[8]+0x8000==Memory.ROM+0x408000);}
  if(t!=4)assert(!memcmp(Memory.ROM,p,n));
  for(unsigned i=0;i<4;i++)assert(!nes_frame(0,0));
  if(t==0){S9xSetPPU(5,0x2105);S9xSetPPU(15,0x2100);for(unsigned i=0;i<3;i++)assert(!nes_frame(0,0));assert(IPPU.RenderedScreenWidth==512);
   S9xSetPPU(1,0x2133);for(unsigned i=0;i<3;i++)assert(!nes_frame(0,0));assert(IPPU.RenderedScreenHeight==448);}

  /* Native detail, interlace and transitions: retain distinct adjacent pixels. */
  for(unsigned y=0;y<478;y++)for(unsigned x=0;x<512;x++)((uint16_t*)(GFX.Screen+y*GFX_PITCH))[x]=(x+y*31)&65535;
  assert(S9xDeinitUpdate(512,478));uint16_t*full=ps_full_pixels();
  assert(full[0]==0&&full[1]==1&&full[477*512+511]==(uint16_t)(511+477*31));
  assert(S9xDeinitUpdate(512,224));assert(full[16*512+1]==1&&full[17*512+1]==1);
  assert(S9xDeinitUpdate(256,239));full=ps_full_pixels();assert(full[2]==1&&full[3]==1);
  if(t==0){
   uint8_t cache[64],valid[1]={1};uint32 colours[16];for(unsigned i=0;i<16;i++)colours[i]=0x100+i;
   for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)cache[y*8+x]=x+1;
   BG.TileAddress=0;BG.TileShift=5;BG.Buffer=cache;BG.Buffered=valid;
   GFX.S=GFX.Screen;GFX.DB=GFX.ZBuffer;GFX.ScreenColorsPre=colours;GFX.PaletteMask=0;GFX.PaletteShift=0;GFX.Z1=GFX.Z2=1;
   memset(GFX.ZBuffer,0,512);DrawHiResTile16(0,0,0,1);
   for(unsigned x=0;x<8;x++)assert(((uint16_t*)GFX.Screen)[x]==0x101+x);
   BG.Buffer=IPPU.TileCache[TILE_4BIT];BG.Buffered=IPPU.TileCached[TILE_4BIT];GFX.ScreenColorsPre=IPPU.ScreenColors;
  }
  if(t>=7){S9xSetPPU(0,0x7034);S9xSetPPU(0,0x701e);S9xSetPPU(0x81,0x701f);assert(Memory.FillRAM[0x3030]&FLG_G);Memory.SRAM[0x10123]=0x5a;}
  uint8_t*state=malloc(PS_STATE_MAX);unsigned size=fc_state_store(state,PS_STATE_MAX);assert(size&&size<=PS_STATE_MAX);
  if(t==3)Memory.C4RAM[8191]=0;
  assert(fc_state_load(state,size));if(t==3)assert(Memory.C4RAM[8191]==123);
  if(t==2){DSP1SetByte(0,0x8000);DSP1SetByte(0x40,0x8000);assert(DSP1GetByte(0x8000)==0&&DSP1GetByte(0x8000)==0x20);}
  if(t>=7){
   uint32 expected[16];FxEmulate(137);memcpy(expected,GSU.avReg,sizeof(expected));
   pocket_player_close();assert(!nes_init(p,n,0,0));assert(fc_state_load(state,size));assert(Memory.SRAM[0x10123]==0x5a);
   FxEmulate(137);assert(!memcmp(expected,GSU.avReg,sizeof(expected)));
   puts("PASS FX3 in-flight GSU pipeline, register and second RAM-bank restore into fresh core");
  }
  state[size-1]^=1;assert(!fc_state_load(state,size));free(state);
  printf("PASS family=%02x map=%02x bytes=%u highres/interlace/state\n",types[t],maps[t],n);fflush(stdout);
  pocket_player_close();free(p);
 }
 assert(!ps_options_valid(24));assert(ps_options_valid(64));assert(!ps_options_valid(192));assert(!ps_options_valid(102u<<9));
 puts("PASS expanded cartridge gates, 6 MiB mapping, chip state and video geometry");return 0;
}
