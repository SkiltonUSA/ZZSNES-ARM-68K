#!/usr/bin/env python3
"""Package the current development build, including install-missing scanline tools."""
import argparse, hashlib, json, shutil, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def package(build, output, fx=None, version=None):
 m=json.loads((ROOT/'docs/current-build.json').read_text());app=m['console']
 if version is None:raise ValueError('An explicit build version is required')
 import re
 if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+-dev',version):raise ValueError('Expected a development build version')
 build=build.resolve();output=output.resolve();output.mkdir(parents=True,exist_ok=True)
 archive=output/(app+'-'+version+'-beta.zip')
 # A fresh staging drawer prevents obsolete/user files entering the archive.
 import tempfile
 with tempfile.TemporaryDirectory(prefix='scanline-package-') as temp:
  drawer=Path(temp)/app;drawer.mkdir()
  for name in ['COPYING3','Credits.txt','LICENSE-Snes9x','LICENSE-puff','LICENSE-xSysInfo',app+'.info',app+'.png']:
   shutil.copy2(ROOT/m['artifact_dir']/name,drawer/name)
  for name in [app,m['player'],'Install-Scanlines']:
   data=(build/name).read_bytes()
   if data[:4]!=bytes.fromhex('000003f3'):raise ValueError('Not an Amiga executable: '+name)
   if name==app or name==m['player']:
    if version.encode() not in data:raise ValueError('Executable version mismatch: '+name)
   (drawer/name).write_bytes(data)
  if app=='ZZSNES':
   if fx is None:raise ValueError('--fx-build is required for ZZSNES')
   data=(fx.resolve()/'ZZSnes-FX').read_bytes()
   if data[:4]!=bytes.fromhex('000003f3'):raise ValueError('Missing Super FX executable')
   (drawer/'ZZSnes-FX').write_bytes(data)
  shutil.copytree(build/'Scanlines',drawer/'Scanlines')
  for name in ['ZZScanlines','ZZScanlinesGUI']:
   if (drawer/'Scanlines'/name).read_bytes()[:4]!=bytes.fromhex('000003f3'):raise ValueError(name)
  shutil.copy2(drawer/(app+'.info'),drawer/'Install-Scanlines.info')
  shutil.copytree(ROOT/'amiga/scanlines',drawer/'Scanlines/Source')
  shutil.copy2(ROOT/'scripts/build_scanlines.py',drawer/'Scanlines/Source/build_scanlines.py')
  (drawer/'Version.txt').write_text(version+' — FPGA fullscreen / Soft window scanlines\n')
  (drawer/'ReadMe.txt').write_text((ROOT/'docs/release-install.txt').read_text())
  shutil.copy2(ROOT/'CHANGELOG.md',drawer/'Changes.md')
  shutil.copy2(ROOT/'docs/scanline-install.txt',drawer/'Scanlines.txt')
  for name in ['ROMs','States','Snapshots']:(drawer/name).mkdir()
  for name in [app,m['player'],'Install-Scanlines','Scanlines/ZZScanlines','Scanlines/ZZScanlinesGUI']+(['ZZSnes-FX']if app=='ZZSNES'else[]):
   (drawer/name).chmod(0o755)
  files=sorted(p for p in drawer.rglob('*') if p.is_file())
  (drawer/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(drawer))+'\n' for p in files))
  with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
   for p in sorted(drawer.rglob('*')):z.write(p,p.relative_to(drawer.parent))
 with zipfile.ZipFile(archive) as z:
  assert z.testzip() is None
  assert not any(Path(n).suffix.lower() in ['.nes','.sfc','.smc','.sav','.zzs'] or Path(n).name=='Library.cfg' for n in z.namelist())
 print('PASS complete package',archive,hashlib.sha256(archive.read_bytes()).hexdigest())
 return archive
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--version',required=True);p.add_argument('--build',type=Path,required=True);p.add_argument('--fx-build',type=Path);p.add_argument('--output',type=Path,default=ROOT/'.context/packages');a=p.parse_args();package(a.build,a.output,a.fx_build,a.version)
