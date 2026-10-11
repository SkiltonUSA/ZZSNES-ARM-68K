// GPL-3.0-or-later. Exact nested exclusive accounting and 32-bit timer wrap.
#include "profile.h"
#include <cassert>
static uint32_t now;
static uint32_t clock_value(){return now;}
int main(){
 ss_profile_reset(clock_value);ss_profile.active=true;
 { SsProfileScope ppu(0);now=10;
   { SsProfileScope same(0);now=20; }
   { SsProfileScope spc(1);now=30;
     { SsProfileScope nested(0);now=40; }
     now=50;
   }
   now=100;
 }
 assert(ss_profile.ticks[0]==80&&ss_profile.ticks[1]==20);
 assert(ss_profile.calls[0]==2&&ss_profile.calls[1]==1&&ss_profile.category==2);
 ss_profile.active=false;{SsProfileScope ignored(0);now=200;}
 assert(ss_profile.ticks[0]==80);
 ss_profile_reset(clock_value);ss_profile.active=true;now=0xfffffff0;
 {SsProfileScope ppu(0);now=16;}
 assert(ss_profile.ticks[0]==32);
 ss_profile_reset(nullptr);ss_profile.active=true;{SsProfileScope ignored(0);}
 assert(!ss_profile.calls[0]);
}
