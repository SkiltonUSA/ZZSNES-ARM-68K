#!/usr/bin/env python3
"""Exercise actual staged fullscreen retirement/cancellation with mocked registers."""
import argparse,json,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('stage',type=Path);p.add_argument('--interval',type=int,choices=(0,250),required=True);a=p.parse_args()
stage=a.stage.resolve();m=json.loads((root/'docs/current-build.json').read_text());source=root/m['snapshot_dir']/'source'
h=(stage/'fullscreen.h').read_text().split('static int em_fullscreen_open(void)')[0].replace('#include "scanout_measure.h"','')
pre = r"""#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t ULONG;typedef uint16_t UWORD;typedef uint8_t UBYTE;
static ULONG tick,unlocks,flips;static int direct_locked,fc_windowed,fc_scanlines;static ULONG fc_core_options,fc_profile_frame;
static void *direct_bitmap,*window;static ULONG direct_lock;
struct Screen {int dummy;};struct Intuition {struct Screen *FirstScreen;};static struct Intuition intuition,*IntuitionBase=&intuition;
static ULONG us(void){return tick;}
static void Forbid(void){} static void Permit(void){}
static void fc_flip_sample(void){flips++;}
static void p96UnlockBitMap(void *b,ULONG l){(void)b;(void)l;assert(direct_locked);unlocks++;}
"""
test = r"""static UWORD regs[128];
static void setup(ULONG now,UWORD ack){
 memset(regs,0,sizeof regs);full_board=(UBYTE *)regs;regs[0x46/2]=ack;
 nf_init(&full_slots);full_fault=0;full_attached=1;direct_locked=1;
 full_origin=0x10000;full_pitch=1280;tick=now;unlocks=flips=0;fc_profile_frame=60;
 assert(full_queue());assert(direct_locked&&full_slots.pending==1);
}
int main(void){
 setup(1000,65535);assert(full_poll());assert(direct_locked&&!unlocks&&!flips);
 tick=1125;regs[0x46/2]=0;assert(full_poll());
 if(EM_ACK_INTERVAL){assert(direct_locked&&!unlocks);tick=1250;assert(full_poll());}
 assert(!direct_locked&&unlocks==1&&flips==1&&full_slots.pending<0&&full_slots.front==1);
 assert(full_poll()&&unlocks==1);
 setup(1000,23);assert(full_poll());regs[0x46/2]=25;tick=1250;
 assert(!full_poll()&&full_fault&&!direct_locked&&unlocks==1&&regs[0x44/2]==2);
 setup(UINT32_MAX-100,23);assert(full_poll());tick=(ULONG)(UINT32_MAX-100+100001u);
 assert(!full_poll()&&full_fault&&!direct_locked&&unlocks==1);
 puts("PASS actual host polling: ACK wrap, delayed retirement, lock lifetime, foreign ACK and timeout cancellation");
}
"""
with tempfile.TemporaryDirectory() as temp:
 f=Path(temp)/'test.c';f.write_text(pre+h+test);binary=Path(temp)/'test'
 subprocess.run(['clang','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-Wno-missing-field-initializers','-DEM_ACK_INTERVAL='+str(a.interval),'-I'+str(stage),'-I'+str(source/'amiga/mesen_nes'),str(f),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
