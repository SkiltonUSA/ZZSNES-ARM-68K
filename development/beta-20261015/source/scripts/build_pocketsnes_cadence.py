#!/usr/bin/env python3
"""Host-only, bounded SNES cadence experiments on the verified C0 payload.
No deploy/launch. Baseline, Thumbnail, Cadence keep 32040 Hz and the 8/6 queue.
"""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
from amiga import PIN
from build_pocketsnes_scanout import replace,sha
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'.context/amiga/pocketsnes-audio-game/Audio'
OUT=ROOT/'.context/amiga/pocketsnes-cadence'
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--variant',choices=['Thumbnail','Baseline','Cadence','Interactive'],action='append',required=True)
 p.add_argument('--frames',type=int,default=1200)
 a=p.parse_args()
 if not 301<=a.frames<=18000:p.error('frames must be 301..18000')
 meta=json.loads((BASE.parent/'builds.json').read_text())['Audio']
 for n,h in meta['sha256'].items():assert sha(BASE/n)==h,n
 records=json.loads((OUT/'builds.json').read_text()) if (OUT/'builds.json').exists() else {}
 for name in a.variant:
  label=f'{name}-{a.frames}';stage=OUT/label;stage.mkdir(parents=True,exist_ok=True)
  for f in BASE.iterdir():
   if f.is_file() and f.suffix in ('.h','.c','.elf'):shutil.copy2(f,stage/f.name)
  shutil.copy2(ROOT/'amiga/pocketsnes/library/cadence.h',stage/'cadence.h')
  s=(stage/'host.c').read_text()
  s=replace(s,'#include "pace_target.h"','#include "pace_target.h"\n#include "cadence.h"\nstatic struct ps_cadence cadence;')
  s=replace(s,'if(!frame_limit || frame_limit>1200)frame_limit=1200;',f'if(!frame_limit || frame_limit>{a.frames})frame_limit={a.frames};')
  cap=a.frames*20000+25000000
  s=s.replace('45000000u',f'{cap}u')
  s=replace(s,'GAME_AUDIO variant=Audio audio=1 immutable=0 scripted_input=1 limit=1200',f'CADENCE variant={name} audio=1 scripted_input=1 limit={a.frames}')
  if name=='Thumbnail':
   s=replace(s,'fc_snapshot_path[0]=0;fc_snapshot_init(rom,size); /* Disable automatic and manual thumbnails. */',
    'fc_snapshot_init(rom,size); /* Isolated automatic thumbnail regression. */')
   s=replace(s,'  frames++;total_core_us+=frame_core_us;',
    '  frames++;total_core_us+=frame_core_us;\n  if(frames==301)puts("THUMBNAIL continued past capture frame");')
  if name in ('Cadence','Interactive'):
   s=replace(s,'next_frame_us=fc_next_deadline(frame_start,next_frame_us,fc_frame_us);',
    'if(!next_frame_us)cadence.valid=0;next_frame_us=ps_cadence_next(&cadence,frame_start,fc_frame_us);')
   s=replace(s,'  if(!core_only){\n   before=us();if(!present(',
    '  if(!core_only && ps_cadence_show(&cadence,us(),fc_frame_us)){\n   before=us();if(!present(')
   s=replace(s,'no frameskip','bounded presentation omission; all core/APU frames retained')
  if name=='Interactive':
   s=replace(s,'\n fc_scripted_input=1;','\n fc_scripted_input=0;')
   s=s.replace('audio=1 scripted_input=1','audio=1 scripted_input=0')
  if a.frames>1200:
   # Detailed short traces are bounded to 1200; long runs keep aggregate stage
   # histograms and a progress sample every 600 emulated frames instead.
   s=replace(s,'  at_audio(frames-1,audio_copy_us,audio_wait_us,audio_samples,audio_submitted,audio_completed,audio_gaps);',
    '  if(frames%600u==0u)printf("CADENCE_PROGRESS frames=%u elapsed_us=%u samples=%u submitted=%u completed=%u gaps=%u flips=%u omitted=%u\\n",frames,(ULONG)(us()-start),audio_samples,audio_submitted,audio_completed,audio_gaps,full_slots.flips,cadence.skipped);')
   s=replace(s,' at_print();',' /* Aggregate-only long run. */')
   s=replace(s,'frames%60==0','frames%600==0')
   f=stage/'frame_profile.h';t=f.read_text()
   t=replace(t,' at_stage(stage,fc_profile_frame,dt);\n','')
   t=replace(t,'at_ack(fc_profile_frame,now);','');f.write_text(t)
  s=replace(s,' rc=0;\ndone:',
   ' printf("CADENCE_RUN frames=%u elapsed_us=%u shown=%u omitted=%u rebases=%u late_max=%u max_streak=%u period_us=%u\\n",frames,(ULONG)(us()-start),direct_challenges,cadence.skipped,cadence.rebases,cadence.late_max,cadence.max_streak,fc_frame_us);\n rc=0;\ndone:')
  (stage/'host.c').write_text(s)
  rel=stage.relative_to(ROOT);exe='ZZSnes-'+label
  defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE=0x2000000','ZZ_CONTROL=0x180000',
   f'ZZ_APP_NAME="{exe}"','ZZ_APP_VERSION="cadence-experiment"','ZZ_MIN_STACK=65536',
   'MB_ALLOCATION=0x2000000u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE',
   'NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
  subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64',
   '-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020',
   '-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',
   *['-D'+d for d in defs],'-I'+str(rel),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug',
   '-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',str(rel/'host.c'),'-lamiga','-o',str(rel/exe)],cwd=ROOT,check=True)
  for n in ['pocket.elf','zz_payload.h','fullscreen.h','flip_plan.h','player_audio.h']:
   assert sha(stage/n)==sha(BASE/n),n
  records[label]={'frames':a.frames,'loop_cap_us':cap,'audio_rate':32040,'audio_slots':8,'audio_prebuffer':6,
   'arm_audio_worker_handoff_unchanged':True,'md5':hashlib.md5((stage/exe).read_bytes()).hexdigest(),
   'sha256':{n:sha(stage/n) for n in ['host.c','cadence.h','pace_target.h','frame_profile.h','player_audio.h','fullscreen.h','flip_plan.h','pocket.elf','zz_payload.h',exe]}}
  print('Built',label,flush=True)
 (OUT/'builds.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':main()
