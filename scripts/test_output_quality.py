#!/usr/bin/env python3
"""Native ownership/worker tests, not physical AHI or audio-quality proof."""
import subprocess,sys
from pathlib import Path
r=Path(__file__).resolve().parents[1];d=r/'development/output-quality-20261030';o=r/'.context/quality-audio-native';o.mkdir(parents=True,exist_ok=True)
subprocess.run(['clang','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(d/'test_queue.c'),'-o',str(o/'queue')],check=True)
subprocess.run([str(o/'queue')],check=True)
subprocess.run([sys.executable,str(d/'test_worker.py')],check=True)
subprocess.run([sys.executable,str(d/'test_stream.py')],check=True)

subprocess.run(['clang','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined',str(d/'test_settings.c'),'-o',str(o/'settings')],check=True)
subprocess.run([str(o/'settings')],cwd=o,check=True)
subprocess.run(['clang++','-std=c++14','-O2','-fsanitize=address,undefined','-I'+str(r/'.context/ax-release-build/player'),str(d/'test_rows.cpp'),'-o',str(o/'rows')],check=True)
subprocess.run([str(o/'rows')],check=True)
