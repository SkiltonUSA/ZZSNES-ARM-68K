#!/usr/bin/env python3
"""Execute trial ACK/lock code with OS stubs. No physical scanout assertion."""
from pathlib import Path
import subprocess, sys
out=Path(sys.argv[1]).resolve()
full=(out/'fullscreen.h').read_text().split('static int full_open(void)')[0]
(out/'flip-under-test.h').write_text(full)
source=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint8_t UBYTE;typedef uint16_t UWORD;typedef uint32_t ULONG;
struct Screen {int dummy;};struct BitMap {int dummy;};
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
 nf_init(&full_slots);
 for(unsigned n=0;n<200000;n++){
  direct_locked=1;assert(full_queue());assert(!full_queue());
  assert(full_poll() && direct_locked); /* No ACK: keep display lock. */
  registers[0x46/2]++;registers[0x4c/2]=(n&1)?0x20:0;
  assert(full_drain() && !direct_locked && full_slots.pending<0);
  assert(full_slots.flips==n+1 && unlocks==n+1);
 }
 assert(trial_ack_blank==100000 && trial_ack_active==100000);
 direct_locked=1;assert(full_queue());now=full_pending_us+100001;
 assert(!full_poll() && full_fault && !direct_locked && full_slots.pending<0);
 assert(registers[0x44/2]==2); /* Pending cancelled before releasing lock. */
 full_fault=0;nf_init(&full_slots);direct_locked=1;assert(full_queue());
 registers[0x46/2]+=2;
 assert(!full_poll() && full_fault && !direct_locked && full_slots.pending<0);
 assert(registers[0x44/2]==2);
 puts("PASS generated serial host: 200000 ACKs, wrap, lock retention, timeout and unexpected ACK cancellation");
}
'''
(out/'flip-host-test.c').write_text(source)
exe=out/'flip-host-test'
subprocess.run(['clang','-std=c99','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(out),out/'flip-host-test.c','-o',exe],check=True)
subprocess.run([exe],check=True)
