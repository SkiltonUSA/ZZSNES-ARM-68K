#!/usr/bin/env python3
"""Release mode must reject diagnostic scheduling combinations before building."""
import subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
for flag in ('--ax-audio-test','--audio-early','--presentation-test=serial','--ack-poll-us=250','--host-service-test=batch4'):
 r=subprocess.run([sys.executable,str(root/'scripts/build_current.py'),'--release-ax',flag],capture_output=True,text=True)
 assert r.returncode==2 and 'release build cannot include diagnostic options' in r.stderr,r.stderr
print('PASS release excludes diagnostic scheduling, polling and batch variants')
