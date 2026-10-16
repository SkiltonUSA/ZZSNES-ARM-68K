#pragma once
#include <stdint.h>
/* MesenCE FX3 cartridge IDs: 0x17, or 0x18 with battery. Detect the
 * header rather than a game title; ordinary Super FX cartridges differ. */
static int snes_fx3(const uint8_t *p,unsigned n){
 const unsigned h=0x7fc0;
 if(n<32768||(n&32767))return 0;
 unsigned map=p[h+21],type=p[h+22];
 unsigned sum=p[h+28]|p[h+29]<<8,check=p[h+30]|p[h+31]<<8;
 return (map==0x20||map==0x30)&&(type==0x17||type==0x18)&&
  (sum^check)==65535&&(p[h+61]&128);
}
#define SNES_FX3_WARNING "Warning: FX3 is not supported.\nThis game cannot run in this version."
