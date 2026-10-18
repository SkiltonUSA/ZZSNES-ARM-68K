#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cartridge.h"
#include "cartridge_warning.h"
#include "library_region.h"
#include "region.h"
#include "cadence.h"
#include "pace_target.h"
static uint8_t rom[0x10200];
static void header(unsigned h,unsigned code,unsigned type){
 rom[h+21]=h==0xffc0?0x31:0x20;rom[h+22]=type;
 rom[h+23]=6;rom[h+25]=code;rom[h+28]=rom[h+29]=255;
 rom[h+61]=128;
}
static void clock_test(unsigned pal,uint32_t offset){
 unsigned period=ps_period_us(pal);struct ps_cadence c={0};uint64_t now=offset,epoch=offset;unsigned count=0;
 for(unsigned frame=0;frame<3000;frame++){
  uint64_t start=now;uint32_t due=ps_cadence_next(&c,(uint32_t)now,period);
  now+=frame%71==70?80000:4000;
  ps_cadence_show(&c,(uint32_t)now,period);
  unsigned wait=ps_pace_target((uint32_t)start,due,period);
  if(now-start<wait)now=start+wait;
  count++;assert(now-epoch>=(uint64_t)count*period); /* Never ahead of regional clock. */
  now+=1000; /* OS/timer jitter must not accumulate into slower emulation. */
  if(frame%301==300){c.valid=0;epoch=now;count=0;} /* pause/state/screenshot scheduling reset */
 }
 assert(now-offset>=(uint64_t)3000*period);
 assert(!ps_pace_target(offset,0,period));
 assert(!ps_pace_target(offset,offset-1,period));
 assert(!ps_pace_target(offset,offset+4*period,period));
 /* Small scheduler delays are compensated, not added to every frame. */
 memset(&c,0,sizeof(c));now=offset;
 for(unsigned i=0;i<3000;i++){
  uint64_t start=now;uint32_t due=ps_cadence_next(&c,(uint32_t)now,period);
  now+=4000;
  unsigned wait=ps_pace_target((uint32_t)start,due,period);
  if(now-start<wait)now=start+wait;
  now+=1000;
 }
 assert(now-offset==(uint64_t)3000*period+1000 && c.rebases==0);
}
int main(void){
 char label[128];
 for(unsigned code=0;code<=12;code++)for(unsigned hi=0;hi<2;hi++){
  memset(rom,0,sizeof(rom));header(hi?0xffc0:0x7fc0,code,0);
  assert(snes_header(rom,65536)==(hi?0xffc0:0x7fc0));
  assert(snes_pal(rom,65536)==(code>=2));
  snes_region_label(label,sizeof(label),code);
  assert(strstr(label,code>=2?"50 fps":"59.92 fps"));
  memmove(rom+512,rom,65536);assert(snes_skip(sizeof(rom))==512);
  assert(snes_pal(rom+512,65536)==(code>=2));
 }
 memset(rom,0,sizeof(rom));header(0x7fc0,2,0x13);
 assert(snes_fx_kind(rom,65536)==1);
 snes_region_label(label,sizeof(label),rom[0x7fd9]);
 assert(!strcmp(label,"PAL: Europe (0x02)\n50 fps target"));
 assert(ps_period_us(1)==20000 && ps_period_us(0)==16689);
 for(unsigned pal=0;pal<2;pal++){
  clock_test(pal,1000);clock_test(pal,0x80000000u);clock_test(pal,0xffff0000u);
 }
 puts("PASS LoROM/HiROM/copier/FX region labels, PAL/NTSC caps, stalls, reset and timer wrap");
}
