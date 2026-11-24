#!/usr/bin/env python3
"""Pixel/depth regression against actual core rendering, no commercial ROMs."""
import argparse, os, subprocess
from pathlib import Path
from compatibility_native import build
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--baseline',action='store_true')
a=p.parse_args()
r=Path(__file__).resolve().parents[1]
out=r/'.context/clipped-tile-review'/('baseline' if a.baseline else 'fixed')
exe=build(r/'tests/amiga/emulator/clipped_tiles.c',out,fx3=True,expanded=True,clip_fix=not a.baseline)
subprocess.run([str(exe)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
