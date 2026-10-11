#!/usr/bin/env python3
"""Build isolated 600-frame SMW diagnostic; external private ROM, never embedded."""
import sys, shutil, difflib
from pathlib import Path
import build_pocketsnes as b
b.OUT=b.ROOT/'.context/amiga/pocketsnes-game-arm'
b.OUT.mkdir(parents=True,exist_ok=True)
shutil.copy2(b.ROOT/'.context/amiga/pocketsnes/libc.a', b.OUT/'libc.a')
original=b.prepare
def prepare():
    sources=original()
    f=b.OUT/'src/soundux.c';old=f.read_text();new=old.replace('sample1 << 4','sample1 * 16')
    for name in ['sample1','sample2']:
        new=new.replace(f'(int32) {name} << shift',f'(int32) {name} * (1 << shift)')
        new=new.replace(f'{name} << shift',f'{name} * (1 << shift)')
    new=new.replace('prev0 << 1','prev0 * 2');f.write_text(new)
    (b.OUT/'active-audio.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'fixture-baseline/soundux.c','game-experiment/soundux.c')))
    return [s if s.name!='bench.c' else b.SRC/'game_arm.c' for s in sources]
b.prepare=prepare
# Reuse the validated compiler/layout contract in an isolated output directory.
code=(b.ROOT/'scripts/build_pocketsnes_arm.py').read_text()
code=code.replace("(OUT/name).write_text(text)", "\n if name=='worker.cpp':text+='\\nextern \\\"C\\\" const void *pocket_game_rom(void){return (const void *)(shared+0x190000);}\\n'\n (OUT/name).write_text(text)")
exec(compile(code,'build_pocketsnes_arm.py','exec'))
