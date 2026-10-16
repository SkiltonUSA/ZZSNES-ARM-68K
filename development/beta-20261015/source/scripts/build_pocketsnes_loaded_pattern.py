#!/usr/bin/env python3
"""Build immutable-pattern controls with SNES load, with/without AHI output.
Preserve the existing FRAME/drain/audio/PAN scheduling; no deploy or launch.
"""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
from amiga import PIN
from build_pocketsnes_scanout import replace,sha
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'.context/amiga/pocketsnes-scanout/C0'
OUT=ROOT/'.context/amiga/pocketsnes-loaded-pattern'
def main():
 global OUT
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--audio-study',action='store_true',help='Build traced Core/Copy/Audio/Wake comparisons')
 parser.add_argument('--variant',choices=['Core','Copy','Audio','Wake','Reserve','Clock'],action='append',help='Select audio-study variants')
 parser.add_argument('--game',action='store_true',help='Keep actual C0 rendering for Audio/Reserve comparisons')
 args=parser.parse_args()
 if args.game and (not args.audio_study or not args.variant or any(n not in ('Audio','Reserve') for n in args.variant)):
  parser.error('--game requires --audio-study and explicit Audio/Reserve variants')
 if args.variant and not args.audio_study:parser.error('--variant requires --audio-study')
 if args.audio_study:OUT=ROOT/('.context/amiga/pocketsnes-audio-game' if args.game else '.context/amiga/pocketsnes-audio-study')
 meta=json.loads((BASE.parent/'builds.json').read_text())['C0']
 for n,h in meta['sha256'].items():assert sha(BASE/n)==h,n
 records=json.loads((OUT/'builds.json').read_text()) if args.variant and (OUT/'builds.json').exists() else {}
 variants=[(n,n in ('Audio','Wake','Reserve','Clock')) for n in args.variant] if args.variant else ([('Core',False),('Copy',False),('Audio',True),('Wake',True)] if args.audio_study else [('Core',False),('Audio',True)])
 for name,audio in variants:
  stage=OUT/name;stage.mkdir(parents=True,exist_ok=True)
  for p in BASE.iterdir():
   if p.is_file() and p.suffix in ('.h','.c','.elf'):shutil.copy2(p,stage/p.name)
  for n in ['scanout_pattern.h','scanout_diagnostic.h','scanout_loaded.h']:
   shutil.copy2(ROOT/'amiga/pocketsnes/library'/n,stage/n)
  s=(stage/'host.c').read_text()
  s=replace(s,'#include "fullscreen.h"','#include "fullscreen.h"\n#include "scanout_diagnostic.h"\n#include "scanout_loaded.h"')
  s=replace(s,' ULONG start=us(),token=(sequence+1)',
    ' if(op==NES_PRESENT){lp_present_requests++;puts("LOAD_PATTERN forbidden PRESENT request");return 0;}\n if(op==NES_FRAME)lp_frames++;\n ULONG start=us(),token=(sequence+1)')
  if args.game:
   s=replace(s,'if(op==NES_PRESENT){lp_present_requests++;puts("LOAD_PATTERN forbidden PRESENT request");return 0;}','if(op==NES_PRESENT)lp_present_requests++;')
  # Replace the complete presenter, leaving the original normal frame loop.
  start=s.index('static int present(ULONG raw_hash,ULONG frame){')
  end=s.index('\n#endif\nstatic uint64_t armclock',start)
  if not args.game:s=s[:start]+'''static int present(ULONG raw_hash,ULONG frame){
 (void)raw_hash;(void)frame;return lp_present();
}'''+s[end:]
  s=replace(s,'if(!player_load_save())goto done;','/* No SRAM reads/writes in this diagnostic. */')
  s=replace(s,'if(fc_resume&&fc_state_transaction(1)!=1)goto done;','/* No save-state resume in this diagnostic. */')
  s=replace(s,'if(!full_open() || !full_measure_refresh())goto done;',
   'if(!full_open() || !full_measure_refresh())goto done;' if args.game else 'if(!full_open() || !full_measure_refresh() || !lp_init())goto done;')
  s=replace(s,'if(!core_only)player_audio_open();',
    'if(!core_only && !player_audio_open())goto done;' if audio else '/* APU still runs on ARM; AHI output disabled for Core comparison. */')
  if not audio and name!='Copy':
   s=replace(s,'if(!core_only&&!player_audio_frame())goto done;','/* No PCM transfer or AHI queue in Core comparison. */')
  s=replace(s,'   else if(key==0x58){if(!(code&128))fc_snapshot_action=1;}','   else if(key==0x58){/* Snapshot disabled for immutable patterns. */}')
  s=replace(s,'   else if(key==0x54||key==0x57){if(!(code&128))fc_state_action=key==0x54?1:2;}','   else if(key==0x54||key==0x57){/* State transactions disabled. */}')
  s=replace(s,'if(!player_choose()){if(quit)rc=0;goto done;}',
   'if(!player_choose()){if(quit)rc=0;goto done;}\n if(core_only){puts("LOAD_PATTERN requires display mode");goto done;}\n if(!frame_limit || frame_limit>1200)frame_limit=1200;\n fc_scripted_input=1;')
  s=replace(s,'printf("ZZSNES_POCKET START ROM=',
    f'puts("LOAD_PATTERN variant={name} audio={int(audio)} immutable=1 scripted_input=1 limit=1200 Q/Esc quits");\n printf("ZZSNES_POCKET START ROM=')
  s=replace(s,'puts("STAGE closing frontend; requesting ARM return next");',
   'if(lp_ready && !full_fault && !lp_finish())rc=20;\n puts("STAGE closing frontend; requesting ARM return next");')
  # Hard wall-clock cap also applies while paused; no indefinite visual test.
  s=replace(s,'while(!quit && (!frame_limit||frames<frame_limit)){',
   'while(!quit && (!frame_limit||frames<frame_limit) && (ULONG)(us()-start)<45000000u){')
  if args.audio_study:
   # Identical bounded host trace in every comparison. No logging inside frame loop.
   shutil.copy2(ROOT/'amiga/pocketsnes/library/audio_trace.h',stage/'audio_trace.h')
   s=replace(s,'#include "frame_profile.h"','#include "audio_trace.h"\n#include "frame_profile.h"')
   f=stage/'frame_profile.h';t=f.read_text()
   t=replace(t,' if(fc_profile_frame<60)return;',
     ' at_stage(stage,fc_profile_frame,dt);\n if(fc_profile_frame<60)return;')
   t=replace(t,'static void fc_flip_sample(void){ULONG now=us();',
     'static void fc_flip_sample(void){ULONG now=us();at_ack(fc_profile_frame,now);')
   f.write_text(t)
   s=replace(s,'  fc_sample(4,us()-frame_start);',
     '  fc_sample(4,us()-frame_start);\n  at_audio(frames-1,audio_copy_us,audio_wait_us,audio_samples,audio_submitted,audio_completed,audio_gaps);')
   s=replace(s,'if(lp_ready && !full_fault && !lp_finish())rc=20;',
     'if(lp_ready && !full_fault && !lp_finish())rc=20;\n at_print();')
   f=stage/'player_audio.h';t=f.read_text()
   if name=='Copy':
    # Same pull/CopyMem into host RAM, no worker, queue publication or AHI calls.
    start=t.index('static int player_audio_frame(void){')
    t=t[:start]+'''static UBYTE copy_only_pcm[4096];
static int player_audio_frame(void){
 ULONG n=ad_get(shared+MB_REPLY,40),start;
 if(n>1024)return 0;if(!n)return 1;
 start=us();syncio->pull(shared+NES_AUDIO,n*4,NULL);
 CopyMem((APTR)(shared+NES_AUDIO),copy_only_pcm,n*4);
 if(player_muted)memset(copy_only_pcm,0,n*4);
 audio_copy_us+=us()-start;audio_samples+=n;return 1;
}
'''
   elif name=='Wake':
    t=replace(t,'Permit();audio_samples+=n;return 1;',
      'Permit();audio_samples+=n;audio_wake();return 1;')
   elif name in ('Reserve','Clock'):
    t=replace(t,'#define AUDIO_SLOTS 8u','#define AUDIO_SLOTS 12u')
    t=replace(t,'#define AUDIO_PREBUFFER 6u','#define AUDIO_PREBUFFER 9u')
    t=replace(t,'8 queue slots, 6-frame prebuffer','12 queue slots, 9-frame prebuffer')
    if name=='Clock':
     # Diagnostic only: measured 16,800 us mode vs NTSC 16,689 us.
     # Slows audio by ~0.66%; not a general-purpose rate synchronizer.
     t=replace(t,'req[slot]->ahir_Frequency=32040;','req[slot]->ahir_Frequency=31828;')
     t=replace(t,'AUDIO task: AHI unit 0, 32040 Hz','AUDIO task: AHI unit 0, 31828 Hz DIAGNOSTIC')
     s=replace(s,' fc_joy_open();',' if(fc_frame_us!=16689u){puts("Clock diagnostic requires NTSC");goto done;}\n fc_joy_open();')
   f.write_text(t)
  if args.game:
   shutil.copy2(ROOT/'amiga/pocketsnes/library/pace_target.h',stage/'pace_target.h')
   s=replace(s,'#include "frame_profile.h"','#include "frame_profile.h"\n#include "pace_target.h"')
   s=replace(s,'(LONG)(next_frame_us-frame_start)>0?next_frame_us-frame_start:0;target=target>2*fc_frame_us?target-2*fc_frame_us:0;', 'ps_pace_target(frame_start,next_frame_us,fc_frame_us);')
   s=replace(s,'while(elapsed<target){','while(elapsed<target && (ULONG)(us()-start)<45000000u){')
   s=replace(s,'fc_snapshot_init(rom,size);','fc_snapshot_path[0]=0;fc_snapshot_init(rom,size); /* Disable automatic and manual thumbnails. */')
   s=s.replace('LOAD_PATTERN variant=','GAME_AUDIO variant=').replace('immutable=1 scripted_input=1','immutable=0 scripted_input=1')
   s=replace(s,'if(lp_ready && !full_fault && !lp_finish())rc=20;',
    'printf("GAME_AUDIO_END frames=%u present_requests=%u\\n",lp_frames,lp_present_requests);')
  (stage/'host.c').write_text(s)
  rel=stage.relative_to(ROOT);exe=('ZZGame-' if args.game else 'ZZPattern-')+name
  defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE=0x2000000','ZZ_CONTROL=0x180000',
   f'ZZ_APP_NAME="{exe}"','ZZ_APP_VERSION="loaded-pattern"','ZZ_MIN_STACK=65536',
   'MB_ALLOCATION=0x2000000u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE',
   'NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
  subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64',
   '-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020',
   '-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',
   *['-D'+d for d in defs],'-I'+str(rel),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug',
   '-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',str(rel/'host.c'),'-lamiga',
   '-o',str(rel/exe)],cwd=ROOT,check=True)
  for n in ['pocket.elf','zz_payload.h','fullscreen.h']:assert sha(stage/n)==sha(BASE/n)
  records[name]={'audio_output':audio,'arm_and_handoff_unchanged_from_C0':True,
   'md5':hashlib.md5((stage/exe).read_bytes()).hexdigest(),
   'sha256':{n:sha(stage/n) for n in ['host.c','fullscreen.h','scanout_pattern.h','scanout_diagnostic.h',
    'scanout_loaded.h','player_audio.h','frame_profile.h','pocket.elf','zz_payload.h',exe]}}
  if args.audio_study:records[name]['sha256']['audio_trace.h']=sha(stage/'audio_trace.h')
  if args.game:records[name]['sha256']['pace_target.h']=sha(stage/'pace_target.h')
  print('Built',name,flush=True)
 (OUT/'builds.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':main()
