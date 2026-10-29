import argparse,json,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('repo',type=Path);p.add_argument('stage',type=Path);p.add_argument('--baseline',action='store_true');a=p.parse_args();repo=a.repo.resolve();stage=a.stage.resolve();m=json.loads((repo/'docs/current-build.json').read_text());source=repo/m['snapshot_dir']/'source'
h=(stage/'fullscreen.h').read_text().split('static int em_fullscreen_open(void)')[0].replace('#include "scanout_measure.h"','')
pre=r'''#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t ULONG;typedef uint16_t UWORD;typedef uint8_t UBYTE;
struct RenderInfo {void *Memory; long BytesPerRow; int RGBFormat;};
static struct RenderInfo mapped;
static ULONG tick,unlocks,locks,flips;static int direct_locked,fc_windowed,fc_scanlines;static ULONG fc_core_options,fc_profile_frame;
static void *direct_bitmap,*window;static ULONG direct_lock;
struct Screen {int dummy;};struct Intuition {struct Screen *FirstScreen;};static struct Intuition intuition,*IntuitionBase=&intuition;
static ULONG us(void){return tick;}
static void Forbid(void){} static void Permit(void){}
static void fc_flip_sample(void){flips++;}
static void p96UnlockBitMap(void *b,ULONG l){(void)b;(void)l;assert(direct_locked);unlocks++;}
static ULONG p96LockBitMap(void *b,UBYTE *ri,unsigned n){(void)b;assert(!direct_locked);assert(n==sizeof(mapped));memcpy(ri,&mapped,n);return ++locks;}
'''
if a.baseline:pre+='static int full_rendering;\n'
test=r'''static UWORD regs[128];static struct Screen screen;
static void setup(ULONG now,UWORD ack){
 memset(regs,0,sizeof regs);full_board=(UBYTE *)regs;regs[0x46/2]=ack;
 nf_init(&full_slots);full_fault=0;full_attached=1;direct_locked=1;
 full_screen=&screen;intuition.FirstScreen=&screen;fc_windowed=0;
 full_origin=0x10000;full_pitch=1280;tick=now;unlocks=flips=locks=0;fc_profile_frame=60;
 full_rendering=0;
 assert(full_queue());assert(direct_locked&&full_slots.pending==1);
}
int main(void){
 setup(1000,65535);unsigned render=full_slots.render;
 /* ARM is still writing free slot when the old PAN ACK arrives. */
 full_rendering=1;regs[0x46/2]=0;tick=1250;assert(full_poll());
 assert(direct_locked&&unlocks==0&&flips==1&&full_slots.pending<0);
 assert(full_slots.render==render&&full_slots.front!=render);
#ifndef BASELINE
 assert(full_render_finish());assert(!direct_locked&&unlocks==1&&!full_rendering);
 mapped.Memory=regs;mapped.BytesPerRow=1280;mapped.RGBFormat=7;
 struct RenderInfo ri;assert(full_render_begin(&ri));assert(direct_locked&&full_rendering&&locks==1);
 full_rendering=0;assert(full_queue());render=full_slots.render;
 /* Reusing the existing lock must not recursively lock P96. */
 assert(full_render_begin(&ri));assert(locks==1&&direct_locked&&full_rendering);
 regs[0x46/2]++;assert(full_poll());assert(direct_locked&&full_slots.render==render);
 assert(full_render_finish());assert(!direct_locked);
 assert(full_relock());assert(direct_locked&&locks==2&&full_origin_known);
 full_unlock();mapped.BytesPerRow=2560;
 assert(!full_relock());assert(direct_locked&&!full_origin_known);full_unlock();
 /* Foreign ACK and timeout cancel PAN, retaining the writing pin until reset. */
 setup(1000,23);full_rendering=1;regs[0x46/2]=25;tick=1250;
 assert(!full_poll()&&full_fault&&direct_locked&&!unlocks&&regs[0x44/2]==2);
 full_unlock();assert(direct_locked); /* Error path cannot unpin. */
 full_rendering=0;full_unlock();assert(!direct_locked&&unlocks==1); /* After reset. */
 setup(UINT32_MAX-100,23);full_rendering=1;tick=(ULONG)(UINT32_MAX-100+100001u);
 assert(!full_poll()&&full_fault&&direct_locked&&!unlocks);
 full_rendering=0;full_unlock();assert(!direct_locked);
 /* Ordinary ACK path, with no render, still unlocks immediately. */
 setup(1000,23);regs[0x46/2]=24;assert(full_poll()&&!direct_locked&&unlocks==1);
 /* Windowed conversion keeps its existing immediate unlock. */
 nf_init(&full_slots);full_fault=0;fc_windowed=1;direct_locked=1;unlocks=0;
 assert(full_queue()&&!direct_locked&&unlocks==1);
#endif
 puts("PASS render lifetime: ACK during conversion, reuse, relocation, timeout, cancellation, window fallback");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 f=Path(tmp)/'test.c';f.write_text(pre+h+test);out=Path(tmp)/'test'
 subprocess.run(['clang','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-Wno-missing-field-initializers',*(['-DBASELINE']if a.baseline else[]),'-I'+str(stage),'-I'+str(source/'amiga/mesen_nes'),str(f),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
