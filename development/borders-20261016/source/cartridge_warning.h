#pragma once
#include <stdint.h>
/* MesenCE FX3 cartridge IDs: 0x17, or 0x18 with battery. Detect the
 * header rather than a game title; ordinary Super FX cartridges differ. */
static int snes_fx_kind(const uint8_t *p,unsigned n){
 const unsigned h=0x7fc0;
 if(n<32768||(n&32767))return 0;
 unsigned map=p[h+21],type=p[h+22];
 unsigned sum=p[h+28]|p[h+29]<<8,check=p[h+30]|p[h+31]<<8;
 if((map!=0x20&&map!=0x30)||(sum^check)!=65535||!(p[h+61]&128)||
    p[h+25]>12||p[h+24]>7||p[h+23]<5||p[h+23]>12||n>0x400000)return 0;
 if(type==0x17||type==0x18)return 3;
 return type==0x13||type==0x14||type==0x15||type==0x1a?1:0;
}
#define SNES_FX3_WARNING "Warning: FX3 compatibility is experimental.\nThe game may not run correctly."

static int snes_fx3(const uint8_t*p,unsigned n){return snes_fx_kind(p,n)==3;}
#define SNES_FX_WARNING "Super FX: experimental compatibility."
