#!/usr/bin/env python3
"""No ROM or hardware required. --baseline demonstrates the pre-fix failure."""
import argparse,subprocess
from pathlib import Path
from compatibility_native import build
p=argparse.ArgumentParser();p.add_argument('--baseline',action='store_true');a=p.parse_args()
r=Path(__file__).resolve().parents[1]
exe=build(r/'tests/amiga/pocketsnes/superfx_timing.c',r/'.context/superfx-timing',baseline=a.baseline)
subprocess.run([exe],check=True)
