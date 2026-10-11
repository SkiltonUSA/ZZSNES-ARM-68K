#!/usr/bin/env python3
"""Package exact verified artifacts. No deployment, ROMs or user preferences."""
from pathlib import Path
import hashlib,json,zipfile
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'artifacts/pocketsnes-library'
DEST=ROOT/'.context/releases';DEST.mkdir(parents=True,exist_ok=True)
VERSION='0.2.0-dev';FOLDER='ZZSnesLibrary'
record=json.loads((SRC/'library-build.json').read_text())
files={}
for source,dest in [('ZZSnesLibrary','ZZSnesLibrary'),('ZZSnes-Pocket','ZZSnes-ui')]:
 data=(SRC/source).read_bytes()
 assert data[:4]==b'\0\0\x03\xf3'
 assert hashlib.sha256(data).hexdigest()==record['files'][source]['sha256']
 files[dest]=data
files['ZZSnesLibrary.info']=(SRC/'ZZSnesLibrary.info').read_bytes()
files['ReadMe.txt']=(ROOT/'docs/install-library.txt').read_bytes()
files['Guide.md']=(ROOT/'amiga/pocketsnes/library/README.md').read_bytes()
files['Library.cfg']=b'ZZSNESUI1\n0 0 2 0 0 0 0 0\n'
files['THIRD_PARTY.md']=(ROOT/'THIRD_PARTY.md').read_bytes()
files['licenses/LICENSE-xSysInfo']=(ROOT/'amiga/pocketsnes/library/LICENSE-xSysInfo').read_bytes()
for name in ['COPYING3','COPYING.RUNTIME']:
 files['licenses/'+name]=(ROOT/'amiga/fceux_player/distribution'/name).read_bytes()
files['licenses/COPYING.picolibc']=(ROOT/'licenses/COPYING.picolibc').read_bytes()
notice=(ROOT/'amiga/pocketsnes/vendor/src/snes9x.h').read_text().split('*/',1)[0]+'*/\n'
files['licenses/COPYING.Snes9x']=notice.encode()
files['build.json']=json.dumps({'version':VERSION,'state_version':2,'files':{n:record['files'][n] for n in ['ZZSnesLibrary','ZZSnes-Pocket']},'upstream_commit':record['upstream_commit'],'source':'https://github.com/SkiltonUSA/ZZSNES-ARM-68K/tree/v0.2.0-dev','roms_included':False},indent=2).encode()+b'\n'
files['SHA256SUMS']=(''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n' for name,data in sorted(files.items()))).encode()
archive=DEST/f'ZZSnes-Library-{VERSION}.zip'
with zipfile.ZipFile(archive,'w') as z:
 entries={FOLDER+'.info':(ROOT/'amiga/fceux_player/distribution/ZZFceux-drawer.info').read_bytes(),**{FOLDER+'/'+n:d for n,d in files.items()},FOLDER+'/ROMs/':b'',FOLDER+'/States/':b''}
 for name,data in sorted(entries.items()):
  info=zipfile.ZipInfo(name,date_time=(2026,10,11,0,0,0));info.create_system=3;info.compress_type=zipfile.ZIP_DEFLATED
  mode=0o40755 if name.endswith('/') else 0o100755 if name.rsplit('/',1)[-1] in ['ZZSnesLibrary','ZZSnes-ui'] else 0o100644
  info.external_attr=mode<<16;z.writestr(info,data)
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None
 assert set(z.namelist())==set(entries)
 for n,data in entries.items():assert z.read(n)==data
 assert not any(Path(n).suffix.lower() in ['.sfc','.smc','.nes','.zzs','.sav','.srm'] for n in z.namelist())
 assert z.read(FOLDER+'/Library.cfg')==b'ZZSNESUI1\n0 0 2 0 0 0 0 0\n'
 for n in ['ZZSnesLibrary','ZZSnes-ui']:assert z.getinfo(FOLDER+'/'+n).external_attr>>16&0o111
checksum=hashlib.sha256(archive.read_bytes()).hexdigest()
(DEST/'SHA256SUMS.txt').write_text(checksum+'  '+archive.name+'\n')
print('PASS verified binaries, ZIP round trip, executable modes, empty library, no ROMs/states')
print(archive,archive.stat().st_size,checksum)
