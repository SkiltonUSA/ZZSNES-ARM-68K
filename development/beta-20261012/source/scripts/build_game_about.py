#!/usr/bin/env python3
"""Rebuild only installed-compatible MUI launchers with the shared About tab."""
import hashlib,json,shutil,subprocess
from pathlib import Path
from build_game_libraries import IMAGE
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'.context/amiga/game-about';COMMON=ROOT/'amiga/fceux_player'
def main():
 record={}
 for name,base in [('ZZNesLibrary',ROOT/'.context/amiga/fceux-compat'),('ZZSnesLibrary',ROOT/'.context/amiga/pocketsnes-regions/build')]:
  stage=OUT/name;stage.mkdir(parents=True,exist_ok=True)
  for p in base.glob('*.h'):shutil.copy2(p,stage/p.name)
  shutil.copy2(base/'launcher.c',stage/'launcher.c')
  s=(COMMON/'library_mui.h').read_text()
  if name=='ZZNesLibrary':
   old='"Header supported: NTSC, mapper %d\\nGameplay compatibility varies by title.",mapper'
   assert s.count(old)==1
   s=s.replace(old,'"Header supported: %s, mapper %d\\nGameplay compatibility varies by title.",fc_rom_pal(p,n,model.entry[selected].path)?"PAL":"NTSC",mapper').replace('NTSC mapper 0 or 90, without trainer or battery.','mapper 0/1/2/4/5/90; no trainer or battery.')
  (stage/'library_mui.h').write_text(s)
  rel=stage.relative_to(ROOT)
  subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',IMAGE,'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-DLIBRARY_PREVIEW',str(rel/'launcher.c'),'-lamiga','-o',str(rel/name)],check=True,cwd=ROOT)
  record[name]={'md5':hashlib.md5((stage/name).read_bytes()).hexdigest(),'sha256':hashlib.sha256((stage/name).read_bytes()).hexdigest(),'bytes':(stage/name).stat().st_size,'source_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in stage.iterdir() if p.suffix in ('.c','.h')}}
  shutil.copy2(COMMON/'Credits.txt',stage/'Credits.txt');shutil.copy2(COMMON/'LICENSE-xSysInfo',stage/'LICENSE-xSysInfo');shutil.copy2(COMMON/'distribution/COPYING3',stage/'COPYING3')
  shutil.copy2(ROOT/'.context/amiga/pocketsnes-library/src/copyright.h',stage/'LICENSE-Snes9x')
  print('Built',name,record[name]['md5'],flush=True)
 (OUT/'build.json').write_text(json.dumps(record,indent=2)+'\n')
if __name__=='__main__':main()
