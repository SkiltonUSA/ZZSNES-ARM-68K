#!/usr/bin/env python3
"""Package the scalable launcher with unchanged, pinned development players."""
from pathlib import Path
import hashlib,json,zipfile
ROOT=Path(__file__).resolve().parents[1]
def digest(b):return hashlib.sha256(b).hexdigest()
def main():
 out=ROOT/'artifacts/development/scalable-launcher-20261019'
 m=json.loads((out/'build.json').read_text());app=m['console'];version=m['version']
 source=ROOT/m['base_archive'];assert digest(source.read_bytes())==m['base_sha256']
 binary=(out/app).read_bytes();assert digest(binary)==m['launcher_sha256']
 assert ('$VER: '+app+' Library '+version).encode() in binary
 with zipfile.ZipFile(source) as z:files={n:z.read(n) for n in z.namelist() if not n.endswith('/')}
 files[app+'/'+app]=binary
 files[app+'/Version.txt']=(version+' — Scalable Workbench launcher\n').encode()
 files[app+'/ReadMe.txt']+=b'\nLauncher update: compact layouts for smaller screens, scrollable pages and resizable windows.\nKeep the supplied player and scanline tools alongside the launcher.\n'
 files.pop(app+'/SHA256SUMS',None)
 files[app+'/SHA256SUMS']=''.join(digest(b)+'  '+n.split('/',1)[1]+'\n' for n,b in sorted(files.items())).encode()
 target=out/(app+'-'+version+'-layout.zip')
 with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED) as z:
  for name,b in sorted(files.items()):
   info=zipfile.ZipInfo(name,(2026,10,19,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=(0o100755 if b[:4]==bytes.fromhex('000003f3') else 0o100644)<<16
   z.writestr(info,b)
  for folder in ['ROMs','States','Snapshots']:
   info=zipfile.ZipInfo(app+'/'+folder+'/',(2026,10,19,0,0,0));info.external_attr=(0o40755<<16)|16;z.writestr(info,b'')
 with zipfile.ZipFile(target) as z:
  assert z.testzip() is None
  assert not any(Path(n).suffix.lower() in {'.nes','.sfc','.smc','.sav','.zzs','.srm'} or Path(n).name in {'Library.cfg','Session.cfg'} for n in z.namelist())
 with zipfile.ZipFile(source) as z:
  changed=[n for n in files if n not in z.namelist() or files[n]!=z.read(n)]
  assert set(changed)=={app+'/'+app,app+'/Version.txt',app+'/ReadMe.txt',app+'/SHA256SUMS'}
 target.with_suffix('.sha256').write_text(digest(target.read_bytes())+'  '+target.name+'\n')
 print('PASS launcher-only package',target.name,len(files),'files; players and scanline tools unchanged')
if __name__=='__main__':main()
