#!/usr/bin/env python3
"""Host-only isolated flip experiments. Never install or launch a player.

Reuse the accepted LUT/STMIA ARM image. 'observe' retains three slots;
'retire' keeps an additional retired slot; 'overlap' also allows presentation
into the free slot before the preceding PAN ACK. Unrestricted overlap is a
diagnostic only: it can starve input polling. 'bounded' permits only one
consecutive overlap before releasing the lock and polling input.
'responsive' explicitly polls queued keyboard events during that opportunity.
'handoff' instead unlocks for input and reacquires/revalidates the bitmap after
each completed ARM presentation, before queuing it. These are comparisons, not
default-player upgrades: measured FPS has not improved reliably.
No firmware/cache changes. Hardware visual acceptance is still required.
"""
import argparse, hashlib, json, shutil, subprocess
from pathlib import Path
from amiga import PIN

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'.context/amiga/pocketsnes-schedule/lut-stmia'
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--mode',choices=['observe','retire','overlap','bounded','responsive','handoff'],required=True)
a=p.parse_args()
OUT=ROOT/'.context/amiga/pocketsnes-flip'/a.mode
OUT.mkdir(parents=True,exist_ok=True)
for f in BASE.iterdir():
 if f.is_file() and f.suffix in ('.h','.c','.elf'):shutil.copy2(f,OUT/f.name)
shutil.copy2(ROOT/'amiga/mesen_nes/flip_plan.h',OUT/'flip_plan.h')

def replace(s,old,new):
 assert s.count(old)==1,(old,s.count(old))
 return s.replace(old,new)

full=(OUT/'fullscreen.h').read_text()
host=(OUT/'host.c').read_text()
full=replace(full,'static void full_write(','''static ULONG full_ack_blank,full_ack_active,full_unlock_blank,full_unlock_active;
static ULONG full_overlap_count,full_hold;
static struct RenderInfo full_render_info;
/* Firmware returns bit21 in the high word of the 0x4c/0x4e register. */
static int full_blank(void){return (*(volatile UWORD *)(full_board+0x4c)&0x20u)!=0;}
static void full_write(''')
full=replace(full,'if(direct_locked){p96UnlockBitMap',
 '''if(direct_locked){if(full_blank())full_unlock_blank++;else full_unlock_active++;p96UnlockBitMap''')
full=replace(full,'if(status==1){fc_flip_sample();',
 '''if(status==1){if(full_blank())full_ack_blank++;else full_ack_active++;fc_flip_sample();''')
host=replace(host,'fc_profile_print();fc_joy_close();',
 '''printf("FLIP_EXPERIMENT mode='''+a.mode+''' ack_blank=%u ack_active=%u unlock_blank=%u unlock_active=%u overlaps=%u\\n",full_ack_blank,full_ack_active,full_unlock_blank,full_unlock_active,full_overlap_count);
 fc_profile_print();fc_joy_close();''')

if a.mode!='observe':
 shutil.copy2(ROOT/'amiga/pocketsnes/library/flip_retire.h',OUT/'flip_plan.h')
 full=replace(full,'full_slots={0,1,-1,0,0}', 'full_slots={0,1,-1,0,0,3}')
 full=full.replace('720','960').replace('slots=3','slots=4')
 full=full.replace('complete 3-slot','complete 4-slot')
 host=host.replace('720*pitch','960*pitch').replace('complete 3-slot','complete 4-slot')

if a.mode in ('overlap','bounded','responsive','handoff'):
 full=replace(full,'full_wait_us+=us()-full_pending_us;full_unlock();return 1;',
              'full_wait_us+=us()-full_pending_us;if(!full_hold)full_unlock();return 1;')
 full=replace(full,'full_pan(full_origin,0);full_slots.pending=-1;full_fault=1;full_unlock();',
              'full_write(0x44,2);full_slots.pending=-1;full_fault=1;\n  /* ARM may still own render memory: retain lock until reset/cleanup. */\n  if(!full_hold){full_pan(full_origin,0);full_unlock();}')
 host=replace(host,'if(!full_drain())return 0;\n if(!full_front())',
              'if(!full_hold && !full_drain())return 0;\n if(!full_front())')
 host=replace(host,'if(!p96GetBitMapAttr(direct_bitmap,P96BMA_ISONBOARD))return 0;',
              'if(!full_hold && !p96GetBitMapAttr(direct_bitmap,P96BMA_ISONBOARD))return 0;')
 host=replace(host,'direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&ri,sizeof(ri));direct_locked=1;',
              '''if(full_hold){ri=full_render_info;full_overlap_count++;}
 else {direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&ri,sizeof(ri));direct_locked=1;full_render_info=ri;}
 /* Never unlock a bitmap while ARM PRESENT can still write to it. */
 full_hold=1;''')
 host=replace(host,'if(!full_queue())return 0;',
              '''if(!full_drain() || !full_queue())return 0;
 full_hold=0;''')
 host=replace(host,'before=us();if(!full_drain())goto done;flip_drain_us+=us()-before;fc_sample(1,us()-before);fc_read_buttons();',
              '''/* The audio producer is the only writer; its worker only frees slots.
   * If no PCM slot is available, drain/unlock before its blocking slow path.
   * A held RenderInfo remains valid until the final ARM reply and PAN ACK. */
  before=us();
  if(!core_only && direct_locked && full_slots.pending>=0 &&
     (!audio_opened || audio_write-audio_read<AUDIO_SLOTS))full_hold=1;
  if(!full_hold && !full_drain())goto done;
  flip_drain_us+=us()-before;fc_sample(1,us()-before);
  if(!direct_locked)fc_read_buttons();''')
 # A failed request retains the lease until launcher resets Core1. Normal
 # shutdown has no lease; avoid a new drain or graphics operation on failure.
 host=replace(host,'if(!full_drain())rc=20;', 'if(full_hold || !full_drain())rc=20;')
 if a.mode in ('bounded','responsive'):
  full=replace(full,'static ULONG full_overlap_count,full_hold;',
               'static ULONG full_overlap_count,full_hold,full_overlap_chain;')
  host=replace(host,'if(!core_only && direct_locked && full_slots.pending>=0 &&',
               'if(!core_only && !full_overlap_chain && direct_locked && full_slots.pending>=0 &&')
  host=replace(host,'(!audio_opened || audio_write-audio_read<AUDIO_SLOTS))full_hold=1;',
               '''(!audio_opened || audio_write-audio_read<AUDIO_SLOTS)){
   full_hold=1;full_overlap_chain=1;
  }else full_overlap_chain=0;''')
 if a.mode=='responsive':
  host=replace(host,'if(!direct_locked)fc_read_buttons();',
               'if(!direct_locked){events();fc_read_buttons();}')
 if a.mode=='handoff':
  host=replace(host,'if(!full_drain() || !full_queue())return 0;\n full_hold=0;',
               '''if(!full_drain())return 0;
 /* Both ARM writing and the preceding PAN are complete. Release the lock
  * for input, then acquire a fresh address before queuing the finished slot.
  * The BitMap remains OS-owned throughout; no ARM access after its reply. */
 full_hold=0;full_unlock();events();fc_read_buttons();
 if(!full_front()){quit=1;return 0;}
 memset(&full_render_info,0,sizeof(full_render_info));
 direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&full_render_info,sizeof(full_render_info));direct_locked=1;
 full_origin_known=0;
 if(!full_render_info.Memory || ((ULONG)full_render_info.Memory&3u) ||
    full_render_info.BytesPerRow!=(LONG)full_pitch ||
    (ULONG)full_render_info.RGBFormat!=direct_native_format ||
    (ULONG)full_render_info.Memory<(ULONG)full_board+0x10000u ||
    (ULONG)full_render_info.Memory>(ULONG)full_board+0x4000000u-960*full_pitch)return 0;
 full_origin=(ULONG)full_render_info.Memory-(ULONG)full_board-0x10000u;full_origin_known=1;
 if(!full_queue())return 0;''')

if a.mode not in ('overlap','bounded','responsive','handoff'):
 full=full.replace('full_overlap_count,full_hold','full_overlap_count').replace('static struct RenderInfo full_render_info;','')
(OUT/'fullscreen.h').write_text(full)
(OUT/'host.c').write_text(host)
relative=OUT.relative_to(ROOT)
defines=['-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_NES','-DZZ_BLOCK_SIZE=0x2000000','-DZZ_CONTROL=0x180000','-DZZ_APP_NAME="ZZSnes-Pocket"','-DZZ_APP_VERSION="flip-'+a.mode+'"','-DZZ_MIN_STACK=65536','-DMB_ALLOCATION=0x2000000u','-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE','-DNES_PLAYER','-DNES_DIRECT','-DNES_FULLSCREEN']
subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*defines,'-I'+str(relative),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',str(relative/'host.c'),'-lamiga','-o',str(relative/'ZZSnes-Pocket')],check=True)
files=['ZZSnes-Pocket','host.c','fullscreen.h','flip_plan.h','pocket.elf','zz_payload.h']
assert (BASE/'pocket.elf').read_bytes()==(OUT/'pocket.elf').read_bytes()
assert (BASE/'zz_payload.h').read_bytes()==(OUT/'zz_payload.h').read_bytes()
(OUT/'flip-build.json').write_text(json.dumps({'mode':a.mode,'files':{n:hashlib.sha256((OUT/n).read_bytes()).hexdigest() for n in files}},indent=2)+'\n')
print('Built',OUT/'ZZSnes-Pocket')
