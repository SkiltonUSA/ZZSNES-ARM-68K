#!/usr/bin/env python3
"""Native PAL/NTSC core cadence, audio and state replay; private ROM arguments.
Requires the cached sanitizer core objects from test_pocketsnes_library_native.py.
"""
from pathlib import Path
import sys,subprocess
import build_pocketsnes as b
ROOT=b.ROOT;b.OUT=ROOT/'.context/amiga/pocketsnes-library';lib=ROOT/'amiga/pocketsnes/library'
out=ROOT/'.context/amiga/pocketsnes-regions';out.mkdir(parents=True,exist_ok=True)
flags=['clang','-std=gnu99','-I'+str(lib),*b.flags(),'-fsanitize=address,undefined','-fno-sanitize-recover=all']
obj=out/'regions.o';subprocess.run([*flags,'-c',ROOT/'tests/amiga/pocketsnes/region_native.c','-o',obj],check=True)
objects=[b.OUT/f'state-native-{i}.o' for i in range(31)]
exe=out/'native';subprocess.run(['clang','-fsanitize=address,undefined',*objects,obj,'-lm','-o',exe],check=True)
unit=out/'regions-unit'
subprocess.run([*flags,'-I'+str(ROOT/'amiga/borders'),'-I'+str(ROOT/'amiga/pocketsnes'),'-Wno-incompatible-pointer-types','-Wno-pointer-sign',ROOT/'tests/amiga/pocketsnes/regions_unit.c',*objects[:30],'-lm','-o',unit],check=True)
subprocess.run([unit],check=True)
results=[]
for name in sys.argv[1:]:
 r=subprocess.run([exe,Path(name).resolve()],capture_output=True,text=True);results.append({'rom':Path(name).name,'returncode':r.returncode,'output':r.stdout+r.stderr});print(r.stdout+r.stderr,flush=True)
import json
(out/'native-results.json').write_text(json.dumps(results,indent=2)+'\n')
assert results and all(r['returncode']==0 for r in results)
