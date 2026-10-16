#!/usr/bin/env python3
"""Allowlisted standalone ZIP; original test ROM only, never user ROMs."""
from pathlib import Path
import argparse,hashlib,json,struct,zipfile
from make_nes_demo import build
ROOT=Path(__file__).resolve().parents[1]
NAME='ZZFceux-0.2.0-rc1'
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-dir',type=Path,default=ROOT/'.context/amiga/fceux-player')
p.add_argument('--out',type=Path,default=ROOT/'.context/dist');a=p.parse_args()
record=json.loads((a.build_dir/'build.json').read_text());binary=(a.build_dir/'ZZFceux').read_bytes()
assert record['scheduling']=='release-profile' and binary[:4]==b'\0\0\x03\xf3'
assert hashlib.sha256(binary).hexdigest()==record['outputs']['ZZFceux']
src=ROOT/'amiga/fceux_player';files={NAME+'/ZZFceux':binary}
for target,source,kind in [(NAME+'.info','ZZFceux-drawer.info',2),(NAME+'/ZZFceux.info','ZZFceux.info',3)]:
 data=(src/'distribution'/source).read_bytes()
 assert data[:4]==b'\xe3\x10\0\1' and data[48]==kind and struct.unpack_from('>I',data,74)[0]==65536
 files[target]=data
for target,source in [('ReadMe.txt',src/'ReadMe.txt'),('COPYING3',ROOT/'LICENSE'),('ThirdParty.txt',ROOT/'THIRD_PARTY.md'),('COPYING.RUNTIME',ROOT/'licenses/COPYING.RUNTIME'),('FCEUX-COPYING',ROOT/'amiga/fceux_bench/vendor/COPYING')]:
 files[NAME+'/'+target]=source.read_bytes()
r=bytearray(build(player_test=True));r[6]=1 # original sound/input fixture, no battery
files[NAME+'/ROMs/Controller-Test.nes']=bytes(r)
files[NAME+'/build.json']=(a.build_dir/'build.json').read_bytes()
files[NAME+'/SHA256SUMS.txt']=''.join(hashlib.sha256(v).hexdigest()+'  '+k+'\n' for k,v in sorted(files.items())).encode()
a.out.mkdir(parents=True,exist_ok=True);path=a.out/(NAME+'.zip')
with zipfile.ZipFile(path,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for name,data in sorted(files.items()):
  info=zipfile.ZipInfo(name,date_time=(2026,10,10,0,0,0));info.create_system=3;info.compress_type=zipfile.ZIP_DEFLATED
  info.external_attr=(0o100755 if name==NAME+'/ZZFceux' else 0o100644)<<16
  z.writestr(info,data)
with zipfile.ZipFile(path) as z:
 assert z.testzip() is None and set(z.namelist())==set(files)
 for n,v in files.items():assert z.read(n)==v
print(path);print(hashlib.sha256(path.read_bytes()).hexdigest())
