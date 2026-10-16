#!/usr/bin/env python3
"""Build an isolated immutable-pattern display diagnostic from verified C0.
No deployment/launch. Keeps C0's ARM payload and reset-before-free lifecycle.
"""
from pathlib import Path
import hashlib,json,shutil,subprocess
from amiga import PIN
from build_pocketsnes_scanout import replace,sha
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'.context/amiga/pocketsnes-scanout/C0'
OUT=ROOT/'.context/amiga/pocketsnes-pattern'
def main():
 meta=json.loads((BASE.parent/'builds.json').read_text())['C0']
 for n,h in meta['sha256'].items():assert sha(BASE/n)==h,n
 OUT.mkdir(parents=True,exist_ok=True)
 for p in BASE.iterdir():
  if p.is_file() and p.suffix in ('.h','.c','.elf'):shutil.copy2(p,OUT/p.name)
 for n in ['scanout_pattern.h','scanout_diagnostic.h']:
  shutil.copy2(ROOT/'amiga/pocketsnes/library'/n,OUT/n)
 p=OUT/'host.c';s=p.read_text()
 s=replace(s,'static uint64_t armclock(void)', '#include "scanout_diagnostic.h"\nstatic uint64_t armclock(void)')
 s=replace(s,'if(!player_load_save())goto done;','/* Diagnostic never reads or writes SRAM files. */')
 s=replace(s,'if(fc_resume&&fc_state_transaction(1)!=1)goto done;','/* No state resume in the immutable-pattern diagnostic. */')
 s=replace(s,'if(!full_open() || !full_measure_refresh())goto done;',
   'if(!full_open() || !full_measure_refresh())goto done;\n rc=sp_run()?0:20;goto done;')
 p.write_text(s)
 rel=OUT.relative_to(ROOT)
 defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE=0x2000000','ZZ_CONTROL=0x180000',
  'ZZ_APP_NAME="ZZScanout-Pattern"','ZZ_APP_VERSION="diagnostic"','ZZ_MIN_STACK=65536',
  'MB_ALLOCATION=0x2000000u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE',
  'NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
 subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64',
  '-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020',
  '-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',
  *['-D'+d for d in defs],'-I'+str(rel),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug',
  '-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',str(rel/'host.c'),'-lamiga',
  '-o',str(rel/'ZZScanout-Pattern')],cwd=ROOT,check=True)
 for n in ['pocket.elf','zz_payload.h']:assert sha(OUT/n)==sha(BASE/n)
 (OUT/'build.json').write_text(json.dumps({'arm_unchanged_from_C0':True,
  'physical_execution':'pending','md5':hashlib.md5((OUT/'ZZScanout-Pattern').read_bytes()).hexdigest(),
  'sha256':{n:sha(OUT/n) for n in ['host.c','fullscreen.h','scanout_pattern.h','scanout_diagnostic.h',
   'pocket.elf','zz_payload.h','ZZScanout-Pattern']}},indent=2)+'\n')
 print('Built isolated ZZScanout-Pattern; no physical execution claimed')
if __name__=='__main__':main()
