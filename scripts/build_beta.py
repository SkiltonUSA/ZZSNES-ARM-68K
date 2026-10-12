#!/usr/bin/env python3
"""Rebuild the Beta 68k host and MUI library from archived, verified inputs.
ARM payload is the archived hardware-tested payload; no deployment or ROMs.
"""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 meta=json.loads((ROOT/'docs/beta-build.json').read_text())
 for rel,digest in json.loads((ROOT/'docs/beta-sha256.json').read_text()).items():
  assert hashlib.sha256((ROOT/rel).read_bytes()).hexdigest()==digest,rel
 snap=Path(meta['snapshot_dir']);source=snap/'source';stage=snap/'build-inputs/player'
 out=Path('.context/beta-rebuild');(ROOT/out).mkdir(parents=True,exist_ok=True)
 nes=meta['console']=='ZZNES';block,control=('0x400000','0x80000') if nes else ('0x2000000','0x180000')
 defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE='+block,'ZZ_CONTROL='+control,'ZZ_APP_NAME="'+meta['console']+'"','ZZ_APP_VERSION="'+meta['version']+'"','ZZ_MIN_STACK=65536','MB_ALLOCATION='+block+'u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
 runtime=json.loads(os.environ.get('BETA_CONTAINER_COMMAND','["podman","--connection","nuflix-converter-root","run","--rm","--platform","linux/amd64"]'))
 base=runtime+['-v',str(ROOT)+':/work','-w','/work',meta['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation']
 subprocess.run(base+['-D'+d for d in defs]+['-I'+str(stage)]+['-I'+str(source/'amiga'/p) for p in ['mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']]+[str(source/'amiga/arm_debug/zz9000/launcher.c'),str(stage/'host.c'),'-lamiga','-o',str(out/meta['player'])],cwd=ROOT,check=True)
 subprocess.run(base+['-DLIBRARY_PREVIEW',str(snap/'build-inputs/library/launcher.c'),'-lamiga','-o',str(out/meta['console'])],cwd=ROOT,check=True)
 print('PASS: Beta player and MUI library built; ARM payload unchanged; no hardware access')
 print('Outputs:',out,'(debug paths differ from the original workspace; use archived artifacts for exact installed bytes)')
if __name__=='__main__':main()
