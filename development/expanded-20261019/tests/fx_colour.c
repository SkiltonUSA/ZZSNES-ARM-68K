/* Original Super FX pixel fixture; no game data. */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "snes9x.h"
#include "memmap.h"
#include "fxemu.h"
#include "fxinst.h"
#include "gfx.h"
extern void ComputeClipWindows(void);
extern FxRegs_s GSU;
extern int nes_init(const uint8_t*,uint32_t,uint16_t*,uint16_t*);
extern void pocket_player_close(void);
void pocket_heap_init(void){}
static unsigned pixel(const uint8_t *p,unsigned x){
 unsigned v=0;for(unsigned b=0;b<8;b++)if(p[(b/2)*16+b%2]&(128u>>x))v|=1u<<b;return v;
}
int main(void){
 uint8_t *rom=calloc(1,0x10000);assert(rom);
 memset(rom+0x7fc0,' ',21);memcpy(rom+0x7fc0,"ZZSNES FX COLOUR TEST",20);
 rom[0]=0x80;rom[1]=0xfe;rom[0x7fd5]=0x20;rom[0x7fd6]=0x13;
 rom[0x7fd7]=6;rom[0x7fd8]=5;rom[0x7fd9]=1;
 rom[0x7fdc]=rom[0x7fdd]=255;rom[0x7ffd]=128;
 assert(!nes_init(rom,0x10000,0,0));assert(Settings.SuperFX);
 S9xSetPPU(0x1b,0x303a);S9xSetPPU(0,0x3038);FxEmulate(0);assert(GSU.vMode==3);
 /* The active raster path writes PPU register RAM, not deferred shadows. */
 S9xSetPPU(0x30,0x2130);ComputeClipWindows();
 assert(IPPU.Clip[1].Count[5]==1);
 assert(IPPU.Clip[1].Left[0][5]>=IPPU.Clip[1].Right[0][5]);
 S9xSetPPU(0xc0,0x2130);ComputeClipWindows();
 assert(IPPU.Clip[0].Count[5]==1);
 assert(IPPU.Clip[0].Left[0][5]>=IPPU.Clip[0].Right[0][5]);
 assert(IPPU.Clip[1].Count[5]==0);
 S9xSetPPU(0,0x2130);ComputeClipWindows();
 assert(IPPU.Clip[0].Count[5]==0&&IPPU.Clip[1].Count[5]==0);
 /* BG1 window mask must follow both screen-enable and window-enable writes. */
 S9xSetPPU(16,0x2126);S9xSetPPU(31,0x2127);S9xSetPPU(3,0x2123);
 for(unsigned screen=0;screen<2;screen++){
  S9xSetPPU(1,0x212c + screen);S9xSetPPU(1,0x212e + screen);ComputeClipWindows();
  assert(IPPU.Clip[screen].Count[0]==1);
  assert(IPPU.Clip[screen].Left[0][0]==16&&IPPU.Clip[screen].Right[0][0]==32);
  S9xSetPPU(0,0x212e + screen);ComputeClipWindows();assert(!IPPU.Clip[screen].Count[0]);
  S9xSetPPU(1,0x212e + screen);S9xSetPPU(0,0x212c + screen);ComputeClipWindows();
  assert(!IPPU.Clip[screen].Count[0]);
 }
 puts("PASS live PPU colour-window and main/sub BG mask register changes");
 unsigned cases=0;
 for(unsigned por=0;por<32;por++)for(unsigned colour=0;colour<256;colour++){
  uint8_t *screen=GSU.apvScreen[0];memset(screen,0xa5,64);
  unsigned before=pixel(screen,0);
  GSU.vPlotOptionReg=por;GSU.vColorReg=colour;GSU.avReg[1]=GSU.avReg[2]=0;
  GSU.pfPlot();
  /* In 8bpp, freeze-high makes only the low nibble select transparency.
     Object layout and dithering must not select the transparency rule. */
  unsigned transparent=!(por&1)&&((por&8)?!(colour&15):!colour);
  unsigned expected=transparent?before:colour;
  if(pixel(screen,0)!=expected){fprintf(stderr,"8bpp POR=%02x COLR=%02x got=%02x expected=%02x\n",por,colour,pixel(screen,0),expected);return 1;}
  for(unsigned x=1;x<8;x++)assert(pixel(screen,x)==((0xa5&(128>>x))?255:0));
  assert(GSU.avReg[1]==1);cases++;
 }
 /* The CPU map exposes both SRAM banks. GSU must see the same 128 KiB;
    SCBR=$25 is used by double-buffered racing graphics. */
 assert(GSU.nRamBanks==2);
 assert(GSU.apvRamBank[0]==Memory.SRAM);
 assert(GSU.apvRamBank[1]==Memory.SRAM+0x10000);
 assert(GSU.apvRomBank[0x71]==Memory.SRAM+0x10000);
 S9xSetPPU(0x25,0x3038);FxEmulate(0);
 assert(GSU.pvScreenBase==Memory.SRAM+0x9400);
 assert(GSU.apvScreen[0]==Memory.SRAM+0x9400);
 memset(Memory.SRAM,0,0x20000);
 GSU.vPlotOptionReg=1;GSU.vColorReg=0xa5;GSU.avReg[1]=240;GSU.avReg[2]=112;
 GSU.pfPlot();
 /* 8bpp, height 128: column tile stride=1024, row tile stride=64. */
 unsigned offset=0x9400+30*1024+14*64;
 assert(offset>0x10000&&offset+64<0x20000);
 assert(pixel(Memory.SRAM+offset,0)==0xa5);
 assert(pixel(Memory.SRAM+offset-0x10000,0)==0);
 puts("PASS Super FX two-bank RAM map and SCBR=$25 second-bank drawing");
 /* GSU ROM banks mirror both halves. A resumed instruction slice may
    legitimately start below $8000; this must execute, not clear GO. */
 uint8_t *code=GSU.apvRomBank[1]+0x100;
 code[0]=0xa0;code[1]=0x5a;code[2]=0;code[3]=1; /* IBT R0,#$5a; STOP */
 GSU.pvRegisters[GSU_PBR]=1;
 GSU.pvRegisters[GSU_SFR]=FLG_G;
 GSU.pvRegisters[30]=0;GSU.pvRegisters[31]=1;
 GSU.vPipe=1;
 assert(FxEmulate(8)>0);
 assert(GSU.avReg[0]==0x5a);
 assert(!(GSU.vStatusReg&FLG_G));
 puts("PASS Super FX lower-half ROM execution across instruction slices");
 pocket_player_close();free(rom);printf("PASS %u Super FX 8bpp colour/transparency cases, adjacent pixels and X advance\n",cases);return 0;
}
