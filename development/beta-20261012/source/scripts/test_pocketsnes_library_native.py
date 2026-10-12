#!/usr/bin/env python3
import sys,hashlib,subprocess
from pathlib import Path
import build_pocketsnes as b
from pocketsnes_library import core_patch,LIB
b.OUT=b.ROOT/'.context/amiga/pocketsnes-library';b.OUT.mkdir(parents=True,exist_ok=True)
sources=[p for p in b.prepare() if p.name!='bench.c']
f=b.OUT/'src/soundux.c';s=f.read_text().replace('sample1 << 4','sample1 * 16')
for name in ['sample1','sample2']:s=s.replace(f'(int32) {name} << shift',f'(int32) {name} * (1 << shift)').replace(f'{name} << shift',f'{name} * (1 << shift)')
f.write_text(s.replace('prev0 << 1','prev0 * 2'));core_patch(b.OUT)
sources+=[LIB/'player.c',b.ROOT/'tests/amiga/pocketsnes/library_native.c']
flags=['clang','-std=gnu99',*b.flags(),'-I'+str(LIB),'-Wno-incompatible-pointer-types','-Wno-pointer-sign','-fsanitize=address,undefined','-fno-sanitize-recover=all']
objects=[]
for i,source in enumerate(sources):
 obj=b.OUT/f'state-native-{i}.o';objects.append(obj);stamp=obj.with_suffix('.sha256');digest=hashlib.sha256(source.read_bytes()+str(flags).encode()+b''.join(p.read_bytes() for p in LIB.glob('*.h'))).hexdigest()
 if not obj.exists() or not stamp.exists() or stamp.read_text()!=digest:
  subprocess.run([*flags,'-c',source,'-o',obj],check=True);stamp.write_text(digest)
subprocess.run(['clang','-fsanitize=address,undefined',*objects,'-lm','-o',b.OUT/'state-native'],check=True)
r=subprocess.run([b.OUT/'state-native',b.ROOT/'.context/amiga/pocketsnes-game/input.sfc',b.OUT/'Library-System-Test.sfc'],capture_output=True,text=True);(b.OUT/'state-native.log').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr);r.check_returncode()
