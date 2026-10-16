#!/usr/bin/env python3
"""Build isolated comparisons or production player from verified compatibility cache."""
import argparse, hashlib, json, shutil, subprocess
from pathlib import Path
from amiga import PIN
from fceux_release import once
from fceux_cadence import apply
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'.context/amiga/fceux-compat'
OUT=ROOT/'.context/amiga/fceux-cadence'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--variant',choices=['Baseline','Cadence','Interactive','Release'],action='append',required=True)
 p.add_argument('--frames',type=int,default=1200);a=p.parse_args()
 if not 301<=a.frames<=18000:p.error('frames must be 301..18000')
 meta=json.loads((BASE/'build.json').read_text())
 for n,v in meta['files'].items():assert sha(BASE/n)==v['sha256'],n
 records=json.loads((OUT/'builds.json').read_text()) if (OUT/'builds.json').exists() else {}
 for name in a.variant:
  release=name=='Release';label=name if release else f'{name}-{a.frames}'
  stage=OUT/label;stage.mkdir(parents=True,exist_ok=True)
  for f in BASE.iterdir():
   if f.is_file() and f.suffix in ('.h','.c','.elf'):shutil.copy2(f,stage/f.name)
  if name!='Baseline':apply(stage)
  else:
   s=(stage/'host.c').read_text();s=once(s,'#include "wait_profile.h"','#include "wait_profile.h"\n#include "cadence.h"\nstatic struct ps_cadence cadence;');(stage/'host.c').write_text(s)
   shutil.copy2(ROOT/'amiga/pocketsnes/library/cadence.h',stage/'cadence.h')
  s=(stage/'host.c').read_text();cap=a.frames*22000+25000000
  if not release:
   s=once(s,' fc_snapshot_init(rom,size);',' fc_snapshot_path[0]=0;fc_snapshot_init(rom,size);')
   s=once(s,' if(!player_load_save())goto done;',' /* Diagnostic: no SRAM reads/writes. */')
   s=once(s,' if(fc_resume&&fc_state_transaction(1)!=1)goto done;',' fc_resume=0;')
   s=once(s,'  if(fc_state_action){int action=fc_state_action;fc_state_action=0;', '  fc_state_action=0;fc_snapshot_action=0;\n  if(fc_state_action){int action=fc_state_action;fc_state_action=0;')
   s=once(s,' while(!quit && (!frame_limit||frames<frame_limit)){',
    f' frame_limit={a.frames};fc_scripted_input={0 if name=="Interactive" else 1};\n while(!quit && frames<frame_limit && (ULONG)(us()-start)<{cap}u){{')
   s=once(s,'while(elapsed<target){',f'while(elapsed<target && (ULONG)(us()-start)<{cap}u){{')
   # Observe steady cadence without continuous serial/file logging.
   s=s.replace('frames%60==0','frames%600==0')
   s=once(s,'  fc_sample(4,us()-frame_start);','  fc_sample(4,us()-frame_start);\n  if(frames%600u==0)printf("CADENCE_PROGRESS frames=%u elapsed_us=%u samples=%u submitted=%u completed=%u gaps=%u flips=%u omitted=%u\\n",frames,(ULONG)(us()-start),audio_samples,audio_submitted,audio_completed,audio_gaps,full_slots.flips,cadence.skipped);')
   s=once(s,' rc=0;\ndone:', ' printf("CADENCE_RUN frames=%u elapsed_us=%u shown=%u omitted=%u rebases=%u late_max=%u max_streak=%u period_us=%u\\n",frames,(ULONG)(us()-start),direct_challenges,cadence.skipped,cadence.rebases,cadence.late_max,cadence.max_streak,fc_frame_us);\n rc=0;\ndone:')
  (stage/'host.c').write_text(s)
  exe='ZZFceux-ui' if release else 'ZZNes-'+label;rel=stage.relative_to(ROOT)
  defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE=0x400000','ZZ_CONTROL=0x80000',f'ZZ_APP_NAME="{exe}"','ZZ_APP_VERSION="regional-cadence"','ZZ_MIN_STACK=65536','MB_ALLOCATION=0x400000u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
  subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*['-D'+d for d in defs],'-I'+str(rel),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',str(rel/'host.c'),'-lamiga','-o',str(rel/exe)],cwd=ROOT,check=True)
  for n in ['fceux.elf','zz_payload.h','fullscreen.h','player_audio.h']:assert sha(stage/n)==sha(BASE/n),n
  records[label]={'frames':a.frames,'loop_cap_us':cap,'audio_rate':32000,'audio_slots':8,'audio_prebuffer':3,'md5':hashlib.md5((stage/exe).read_bytes()).hexdigest(),'sha256':{n:sha(stage/n) for n in ['host.c','cadence.h','player_audio.h','fullscreen.h','fceux.elf','zz_payload.h',exe]}}
  print('Built',label,flush=True)
 (OUT/'builds.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':main()
