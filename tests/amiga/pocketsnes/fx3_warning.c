#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "cartridge.h"
#include "cartridge_warning.h"
static uint8_t rom[65536+512];
int main(void){
 for(unsigned skip=0;skip<=512;skip+=512)for(unsigned type=0;type<256;type++){
  memset(rom,0,sizeof(rom));uint8_t*p=rom+skip;unsigned h=0x7fc0;
  p[h+21]=0x20;p[h+22]=type;p[h+23]=6;p[h+25]=1;
  p[h+28]=p[h+29]=255;p[h+61]=128;
  assert(snes_skip(65536+skip)==skip);
  assert(snes_fx3(p,65536)==(type==0x17||type==0x18));
  int fx=type==0x13||type==0x14||type==0x15||type==0x1a||type==0x17||type==0x18;
  assert(!!snes_fx_kind(p,65536)==fx);
  assert((snes_header(p,65536)>=0)==(type<=2||fx));
  if(type==0x17||type==0x18){
   assert(snes_header(p,65536)==0x7fc0); /* Advisory, not a launch veto. */
   p[h+21]=0x30;assert(snes_fx3(p,65536));
   p[h+21]=0x23;assert(!snes_fx3(p,65536));p[h+21]=0x20;
   p[h+28]=0;assert(!snes_fx3(p,65536));p[h+28]=255;
   p[h+61]=0;assert(!snes_fx3(p,65536));
  }
 }
 for(unsigned n=0;n<32768;n++)assert(!snes_fx3(rom,n));
 assert(!snes_fx3(rom,32769));
 puts("PASS FX3 warning: all cartridge IDs, copier headers, invalid headers, truncated input; FX3 launch allowed");
}
