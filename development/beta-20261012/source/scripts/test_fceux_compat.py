from pathlib import Path
import subprocess,sys,json
import fceux_compat as c
sources=c.prepare();flags=['-std=c++14','-DDINGUX','-DFC_STATES','-O2','-g','-fwrapv','-fno-exceptions','-fno-rtti','-Wno-deprecated-register','-Wno-writable-strings','-Wno-non-c-typedef-for-linkage','-I'+str(c.OUT),'-I'+str(c.b.SRC)]
for mode,extra in [('native',[]),('fixed',['-DFC_ARM']),('sanitize',['-fsanitize=address,undefined','-fno-sanitize=array-bounds,pointer-overflow','-fno-sanitize-recover=all'])]:
 subprocess.run(['clang++',*flags,*extra,*sources,c.ROOT/'tests/amiga/fceux_player/compat_native.cpp','-o',c.OUT/mode],check=True)
