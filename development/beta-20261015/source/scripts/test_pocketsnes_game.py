#!/usr/bin/env python3
"""Private ROM native reference; no deployment or ROM publication."""
import argparse,subprocess,re,difflib,hashlib
import build_pocketsnes as b
p=argparse.ArgumentParser(description=__doc__);p.add_argument('rom');a=p.parse_args()
b.OUT=b.ROOT/'.context/amiga/pocketsnes-game'
sources=[s for s in b.prepare() if s.name!='bench.c']+[b.SRC/'game_native.c']
# Real audio exercises signed BRR samples that the silent fixture never reaches.
# Keep this experiment's patch separate from the saved hardware benchmark.
f=b.OUT/'src/soundux.c';old=f.read_text();new=old
new=new.replace('sample1 << 4','sample1 * 16')
for name in ['sample1','sample2']:
 new=new.replace(f'(int32) {name} << shift',f'(int32) {name} * (1 << shift)')
 new=new.replace(f'{name} << shift',f'{name} * (1 << shift)')
new=new.replace('prev0 << 1','prev0 * 2')
f.write_text(new)
(b.OUT/'active-audio.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'fixture-baseline/soundux.c','game-experiment/soundux.c')))

flags=['clang','-std=gnu99',*b.flags(),'-Wno-incompatible-pointer-types','-Wno-pointer-sign','-fsanitize=address,undefined','-fno-sanitize-recover=all']
objects=[]
for i,source in enumerate(sources):
 obj=b.OUT/(str(i)+'-game-native.o');stamp=obj.with_suffix('.sha256');objects.append(obj)
 digest=hashlib.sha256(source.read_bytes()+str(flags).encode()).hexdigest()
 if not stamp.exists() or stamp.read_text()!=digest:
  subprocess.run([*flags,'-c',source,'-o',obj],check=True);stamp.write_text(digest)
subprocess.run(['clang','-fsanitize=address,undefined',*objects,'-lm','-o',b.OUT/'game-native'],check=True)
r=subprocess.run([b.OUT/'game-native',a.rom],text=True,capture_output=True);(b.OUT/'game-native.log').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr);r.check_returncode()
