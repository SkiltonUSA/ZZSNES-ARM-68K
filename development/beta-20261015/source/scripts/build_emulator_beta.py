#!/usr/bin/env python3
"""Package the accepted NES/SNES cadence hosts and current Beta MUI libraries.

Uses verified local source caches. No hardware access; no ROMs packaged.
"""
import hashlib,json,shutil,subprocess
from pathlib import Path
from amiga import PIN
import build_game_about
from fceux_release import once
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'.context/amiga/emulator-beta/build'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 build_game_about.OUT=OUT/'libraries';build_game_about.main()
 records={}
 for console,version,base,exe,block,control in [
  ('ZZNES','0.5.0 Beta',ROOT/'.context/amiga/fceux-cadence/Release','ZZFceux-ui','0x400000','0x80000'),
  ('ZZSNES','0.4.0 Beta',ROOT/'.context/amiga/pocketsnes-scanout/C0','ZZSnes-ui','0x2000000','0x180000')]:
  stage=OUT/console;stage.mkdir(parents=True,exist_ok=True)
  meta=json.loads((base.parent/'builds.json').read_text())['Release' if console=='ZZNES' else 'C0']
  for n,h in meta['sha256'].items():assert sha(base/n)==h,n
  for p in base.iterdir():
   if p.is_file() and p.suffix in ('.h','.c','.elf'):shutil.copy2(p,stage/p.name)
  s=(stage/'host.c').read_text()
  if console=='ZZSNES':
   for name in ['cadence.h','pace_target.h']:shutil.copy2(ROOT/'amiga/pocketsnes/library'/name,stage/name)
   s=once(s,'#include "fullscreen.h"','#include "fullscreen.h"\n#include "cadence.h"\n#include "pace_target.h"\nstatic struct ps_cadence cadence;')
   s=once(s,'next_frame_us=fc_next_deadline(frame_start,next_frame_us,fc_frame_us);','if(!next_frame_us)cadence.valid=0;next_frame_us=ps_cadence_next(&cadence,frame_start,fc_frame_us);')
   s=once(s,'  if(!core_only){\n   before=us();if(!present(','  if(!core_only && ps_cadence_show(&cadence,us(),fc_frame_us)){\n   before=us();if(!present(')
   s=once(s,'target=(LONG)(next_frame_us-frame_start)>0?next_frame_us-frame_start:0;target=target>2*fc_frame_us?target-2*fc_frame_us:0;','target=ps_pace_target(frame_start,next_frame_us,fc_frame_us);')
   s=once(s,'if(!core_only)player_audio_open();','if(!core_only && !player_audio_open())goto done;')
   s=s.replace('no frameskip','bounded picture omission; every core/APU frame retained')
  # Production path: no forced input, cap, disabled state or disabled capture.
  for token in ['fc_snapshot_init(rom,size);','if(!player_load_save())goto done;','if(fc_resume&&fc_state_transaction(1)!=1)goto done;','while(!quit && (!frame_limit||frames<frame_limit)){']:
   assert token in s,(console,token)
  assert '\n fc_scripted_input=1;' not in s and 'fc_snapshot_path[0]=0;fc_snapshot_init' not in s
  (stage/'host.c').write_text(s)
  rel=stage.relative_to(ROOT)
  defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE='+block,'ZZ_CONTROL='+control,f'ZZ_APP_NAME="{console}"',f'ZZ_APP_VERSION="{version}"','ZZ_MIN_STACK=65536','MB_ALLOCATION='+block+'u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
  subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*['-D'+d for d in defs],'-I'+str(rel),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',str(rel/'host.c'),'-lamiga','-o',str(rel/exe)],cwd=ROOT,check=True)
  for n in ['zz_payload.h','fullscreen.h','player_audio.h']:assert sha(stage/n)==sha(base/n),n
  lib='ZZNesLibrary' if console=='ZZNES' else 'ZZSnesLibrary'
  package=OUT/'transfer'/console;package.mkdir(parents=True,exist_ok=True)
  shutil.copy2(stage/exe,package/exe);shutil.copy2(OUT/'libraries'/lib/lib,package/console)
  for n in ['Credits.txt','COPYING3','LICENSE-xSysInfo','LICENSE-Snes9x']:shutil.copy2(OUT/'libraries'/lib/n,package/n)
  shutil.copy2(ROOT/'.context/amiga/emulator-beta'/(('nes-' if console=='ZZNES' else 'snes-')+lib+'.info'),package/(console+'.info'))
  (package/'Version.txt').write_text(console+' '+version+'\n12 October 2026\n')
  (package/'ReadMe.txt').write_text(console+' '+version+'\n\nOpen '+console+' from Workbench.\nROMs, States and Snapshots are stored in this drawer.\nP pauses; Q or Escape returns to the library.\nF5 saves; F8 loads; F9 captures library artwork.\n')
  records[console]={'version':version,'player':exe,'normal_input_and_states':True,'base':str(base.relative_to(ROOT)),'sources':{p.name:sha(p) for p in stage.iterdir() if p.suffix in ('.c','.h','.elf')},'files':{p.name:{'bytes':p.stat().st_size,'md5':hashlib.md5(p.read_bytes()).hexdigest(),'sha256':sha(p)} for p in package.iterdir() if p.is_file()}}
  print('Built',console,version,flush=True)
 (OUT/'build.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':main()
