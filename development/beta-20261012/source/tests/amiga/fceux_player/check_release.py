#!/usr/bin/env python3
"""Test actual generated input logic and timing bins; no Amiga hardware."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[3]
out=root/'.context/amiga/fceux-player'
joystick=(root/'amiga/fceux_player/joystick.h').read_text()
a=joystick.index('static UBYTE fc_joy_decode');b=joystick.index('static UBYTE fc_read_buttons')
# Standard V40 lowlevel ABI constants, from Commodore's libraries/lowlevel.h.
prefix='''#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef uint32_t ULONG;typedef uint8_t UBYTE;
#define JP_TYPE_MASK (15u<<28)
#define JP_TYPE_JOYSTK (3u<<28)
#define JP_TYPE_GAMECTLR (1u<<28)
#define JPF_JOY_UP (1u<<3)
#define JPF_JOY_DOWN (1u<<2)
#define JPF_JOY_LEFT (1u<<1)
#define JPF_JOY_RIGHT (1u<<0)
#define JPF_BUTTON_RED (1u<<22)
#define JPF_BUTTON_BLUE (1u<<23)
#define JPF_BUTTON_GREEN (1u<<20)
#define JPF_BUTTON_PLAY (1u<<17)
static ULONG clock_us;
static ULONG us(void){return clock_us;}
'''
main='''
int main(void){
 const ULONG flags[8]={JPF_BUTTON_RED,JPF_BUTTON_BLUE,JPF_BUTTON_GREEN,JPF_BUTTON_PLAY,JPF_JOY_UP,JPF_JOY_DOWN,JPF_JOY_LEFT,JPF_JOY_RIGHT};
 for(unsigned n=0;n<256;n++){
  ULONG flags_in=0;for(unsigned i=0;i<8;i++)if(n&(1u<<i))flags_in|=flags[i];
  assert(fc_joy_decode(JP_TYPE_GAMECTLR|flags_in)==n);
  assert(fc_joy_decode(JP_TYPE_JOYSTK|flags_in)==(n&~12u));
  for(unsigned type=0;type<16;type++)if(type!=1&&type!=3)assert(!fc_joy_decode((type<<28)|flags_in));
 }
 fc_sample(0,30000);assert(!fc_stages[0].n);
 fc_profile_frame=60;
 const ULONG times[6]={1000,8000,17000,20000,34000,34001};
 for(unsigned i=0;i<6;i++)fc_sample(0,times[i]);
 for(unsigned i=0;i<6;i++)assert(fc_stages[0].bins[i]==1);
 assert(fc_stages[0].n==6&&fc_stages[0].max==34001&&fc_stages[0].sum==114001);
 clock_us=0xfffffff0u;fc_flip_sample();clock_us=16;fc_flip_sample();
 assert(fc_stages[5].n==1&&fc_stages[5].sum==32);
 fc_profile_print();
 puts("PASS all pad combinations, reject mouse/unknown, timing bins/startup exclusion/timer wrap");
}
'''
p=out/'release-host-test.c';p.write_text(prefix+joystick[a:b]+(root/'amiga/fceux_player/frame_profile.h').read_text()+main)
subprocess.run(['clang','-std=c99','-O2','-g','-fsanitize=address,undefined',p,'-o',out/'release-host-test'],check=True)
subprocess.run([out/'release-host-test'],check=True)
host=(out/'host.c').read_text()
assert 'fc_scripted_input)input=frames>=89&&frames<99?8:0' in host
assert 'flip_drain_us+=us()-before;fc_sample(1,us()-before);fc_read_buttons();' in host
