#!/usr/bin/env python3
"""Native ownership/worker tests, not physical AHI or audio-quality proof."""
import subprocess,sys
from pathlib import Path
r=Path(__file__).resolve().parents[1];d=r/'development/ax-audio-20261029';o=r/'.context/ax-audio-native';o.mkdir(parents=True,exist_ok=True)
subprocess.run(['clang','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(d/'test_queue.c'),'-o',str(o/'queue')],check=True)
subprocess.run([str(o/'queue')],check=True)
subprocess.run([sys.executable,str(d/'test_worker.py')],check=True)
subprocess.run([sys.executable,str(d/'test_stream.py')],check=True)
