#!/usr/bin/env python3
"""Exercise generated fullscreen ACK/lock logic, including ACK during PRESENT.
OS stubs model ownership only; they do not establish physical scanout timing.
"""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[3]
out=Path(sys.argv[1]).resolve()
full=(out/'fullscreen.h').read_text().split('static int full_open(void)')[0]
(out/'flip-under-test.h').write_text(full)
source=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint8_t UBYTE;typedef uint16_t UWORD;typedef uint32_t ULONG;
struct Screen {int dummy;};struct BitMap {int dummy;};struct RenderInfo {void *Memory;};
struct Intuition {struct Screen *FirstScreen;} intuition,*IntuitionBase=&intuition;
static int direct_locked,direct_lock;static struct BitMap *direct_bitmap;
static unsigned unlocks;static ULONG now;
static ULONG us(void){return now++;}
static void Forbid(void){}static void Permit(void){}
static void fc_flip_sample(void){}
static void p96UnlockBitMap(struct BitMap *b,int lock){(void)b;(void)lock;unlocks++;}
#include "flip-under-test.h"
static UWORD registers[256];
int main(void){
 full_board=(UBYTE*)registers;full_attached=1;full_pitch=1280;full_origin=0x10000;
 nf_init(&full_slots);direct_locked=1;
 assert(full_queue());assert(full_slots.pending==1);
 full_hold=1;
 registers[0x46/2]++;
 assert(full_poll());assert(full_slots.pending<0 && full_slots.front==1);
 assert(direct_locked && !unlocks); /* ACK cannot revoke ARM's render lease. */
 assert(full_queue());full_hold=0;
 registers[0x46/2]++;
 assert(full_poll());assert(!direct_locked && unlocks==1);
 /* Timeout or unexpected ACK while ARM may write must retain lock for reset. */
 direct_locked=1;full_hold=1;assert(full_queue());
 now=full_pending_us+100001;
 assert(!full_poll());assert(full_fault && direct_locked && unlocks==1);
 full_unlock();assert(unlocks==2); /* only after simulated Core1 reset */
 full_fault=0;nf_init(&full_slots);direct_locked=1;assert(full_queue());full_hold=1;
 registers[0x46/2]+=2;
 assert(!full_poll());assert(full_fault && direct_locked && unlocks==2);
 puts("PASS generated overlap host: ACK during ARM write, normal unlock, timeout/error retention");
}
'''
host=(out/'host.c').read_text()
if 'full_hold=0;full_unlock();events();fc_read_buttons();' in host:
 section=host[host.index(' if(!full_drain())return 0;\n /* Both ARM'):host.index(' /* Usual frames stay locked through ACK;')]
 # The Amiga is 32-bit; make native pointer truncation explicit in this model.
 section=section.replace('(ULONG)full_render_info.Memory','(ULONG)(uintptr_t)full_render_info.Memory').replace('(ULONG)full_board','(ULONG)(uintptr_t)full_board')
 source=source.replace('struct RenderInfo {void *Memory;}',
                       'struct RenderInfo {void *Memory;int32_t BytesPerRow;ULONG RGBFormat;}')
 source=source.replace('#include <stdio.h>','#include <stdio.h>\n#include <string.h>\ntypedef int32_t LONG;')
 helper='''
static ULONG direct_native_format=9,polled,relocks;
static LONG next_pitch=1280;
static int quit;
static void events(void){assert(!direct_locked);polled++;}
static void fc_read_buttons(void){assert(!direct_locked);polled++;}
static int p96LockBitMap(struct BitMap *b,UBYTE *p,unsigned size){
 struct RenderInfo *ri=(struct RenderInfo*)p;(void)b;assert(size==sizeof(*ri));
 assert(!direct_locked && polled==2);relocks++;
 ri->Memory=(void*)(uintptr_t)((ULONG)(uintptr_t)full_board+0x40000u);
 ri->BytesPerRow=next_pitch;ri->RGBFormat=9;return 1;
}
static int handoff(void){
'''+section+'''return 1;}
'''
 source=source.replace('static UWORD registers[256];','static UWORD registers[256];\n'+helper)
 test='''
 {struct Screen screen;full_screen=&screen;intuition.FirstScreen=&screen;
 full_fault=0;full_hold=0;nf_init(&full_slots);direct_locked=1;assert(full_queue());
 full_hold=1;registers[0x46/2]++;
 assert(handoff());assert(polled==2 && relocks==1 && direct_locked && !full_hold);
 assert(full_slots.pending==2 && full_origin==0x30000u && full_origin_known);
 full_fault=0;full_hold=0;nf_init(&full_slots);direct_locked=1;assert(full_queue());
 full_hold=1;registers[0x46/2]++;polled=relocks=0;next_pitch=1296;
 assert(!handoff());assert(direct_locked && !full_origin_known && full_slots.pending<0);
 /* Failed validation cannot queue an address computed with the old pitch. */
 puts("PASS actual handoff: retain render lease through ACK, unlock for input, relock/revalidate relocated address before PAN");}
'''
 source=source.replace(' puts("PASS generated overlap host:',test+' puts("PASS generated overlap host:')
if 'full_overlap_chain' in host:
 admission=host[host.index('  before=us();\n  if(!core_only && !full_overlap_chain'):host.index('  before=us();if(!core_only&&!player_audio_frame())')]
 admission=admission.replace('goto done','assert(0)')
 helper='''
#define AUDIO_SLOTS 8u
static int core_only,audio_opened=1;
static ULONG audio_write,audio_read,flip_drain_us,input_polls,key_polls;
static void fc_sample(unsigned a,ULONG b){(void)a;(void)b;}
static void fc_read_buttons(void){input_polls++;}
static void events(void){key_polls++;}
static void admission(void){ULONG before;
'''+admission+'}\n'
 source=source.replace('static UWORD registers[256];','static UWORD registers[256];\n'+helper)
 test='''
 full_fault=0;full_hold=0;nf_init(&full_slots);direct_locked=1;assert(full_queue());
 admission();assert(full_hold && full_overlap_chain && !input_polls);
 registers[0x46/2]++;assert(full_poll());assert(direct_locked);
 assert(full_queue());full_hold=0;registers[0x46/2]++;
 admission();assert(!full_hold && !full_overlap_chain && !direct_locked && input_polls==1);
 /* A full PCM queue always takes the unlocked slow path. */
 direct_locked=1;assert(full_queue());registers[0x46/2]++;audio_write=AUDIO_SLOTS;
 admission();assert(!full_hold && !direct_locked && input_polls==2);
 puts("PASS bounded admission forces an input opportunity and unlock before full audio queue");
'''
 source=source.replace(' puts("PASS generated overlap host:',test+' puts("PASS generated overlap host:')
 if 'if(!direct_locked){events();fc_read_buttons();}' in host:
  source=source.replace('puts("PASS bounded admission', 'assert(key_polls==2);puts("PASS bounded admission')
(out/'flip-host-test.c').write_text(source)
exe=out/'flip-host-test'
subprocess.run(['cc','-std=c99','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(out),str(out/'flip-host-test.c'),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
