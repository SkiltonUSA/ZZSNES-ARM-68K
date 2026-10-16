#!/usr/bin/env python3
"""Sanitizer checks for thumbnail encoding and local snapshot lifecycle."""
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'.context/amiga/game-snapshots/tests';OUT.mkdir(parents=True,exist_ok=True)
logs=[]
for name in ['snapshot','snapshot_host','session']:
    binary=OUT/name
    subprocess.run(['clang','-std=gnu99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',
        '-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'amiga/fceux_player'),
        str(ROOT/f'tests/amiga/fceux_player/{name}.c'),'-o',str(binary)],check=True)
    r=subprocess.run([binary],capture_output=True,text=True);logs.append(r.stdout+r.stderr)
    (OUT/'tests.log').write_text('\n'.join(logs));print(logs[-1],end='');r.check_returncode()
