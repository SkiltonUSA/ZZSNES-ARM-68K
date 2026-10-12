#!/usr/bin/env python3
"""Build and exercise real opt-in state core; no hardware access."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[3];out=root/'.context/amiga/fceux-player'
sources=[out/x for x in ['x6502.cpp','ppu.cpp','sound.cpp','filter.cpp','cart.cpp','90.cpp']]
flags=['-std=c++14','-O2','-g','-DDINGUX','-DFC_STATES','-fwrapv','-fno-exceptions','-fno-rtti','-Wno-deprecated-register','-Wno-writable-strings','-Wno-non-c-typedef-for-linkage','-I'+str(root/'amiga/fceux_bench'),'-I'+str(out)]
binary=out/'state-test'
subprocess.run(['clang++',*flags,'-fsanitize=address,undefined','-fno-sanitize=array-bounds,pointer-overflow','-fno-sanitize-recover=all',*sources,root/'amiga/fceux_bench/core.cpp',root/'tests/amiga/fceux_player/states.cpp','-o',binary],check=True)
for rom in sys.argv[1:]:
 subprocess.run([binary,rom],check=True)
 state=out/'cold-state.zzs'
 before=subprocess.check_output([binary,rom,'save',state])
 after=subprocess.check_output([binary,rom,'load',state])
 assert before==after,'cold restore mismatch: '+str(rom)
 print('PASS fresh-process restore:',Path(rom).name)
