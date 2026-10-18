#!/usr/bin/env python3
"""Rebuild the current development MUI launcher only. No ARM build or deployment."""
import json,os,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 m=json.loads((ROOT/'docs/current-build.json').read_text());out=ROOT/'.context/launcher-build';lib=out/'library';snapshot=ROOT/m['library_snapshot_dir']
 if lib.exists():shutil.rmtree(lib)
 shutil.copytree(snapshot/'build-inputs',lib)
 for header in (snapshot/'source').glob('*.h'):shutil.copy2(header,lib/header.name)
 shutil.copytree(snapshot/'source/zip',lib/'zip',dirs_exist_ok=True)
 if m['console']=='ZZSNES':shutil.copytree(ROOT/'development/thin-scanlines-20261016/source',lib,dirs_exist_ok=True)
 shutil.copytree(ROOT/'development/optimisation-20261018/source',lib,dirs_exist_ok=True)
 p=lib/'library_mui.h';p.write_text(p.read_text().replace('0.7.1 Beta','0.7.2-dev').replace('0.6.1 Beta','0.6.2-dev'))
 runtime=json.loads(os.environ.get('BETA_CONTAINER_COMMAND','["podman","--connection","nuflix-converter-root","run","--rm","--platform","linux/amd64"]'))
 subprocess.run(runtime+['-v',str(ROOT)+':/work','-w','/work',m['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',str((lib/'launcher.c').relative_to(ROOT)),str((lib/'zip/puff.c').relative_to(ROOT)),'-lamiga','-o',str((out/m['console']).relative_to(ROOT))],check=True)
 print('PASS launcher',out/m['console'])
if __name__=='__main__':main()
