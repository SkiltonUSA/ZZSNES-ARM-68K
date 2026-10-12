#!/usr/bin/env python3
"""Exercise the actual adapter, cartridge rejection, and reused host helpers."""
from pathlib import Path
import subprocess,sys
ROOT=Path(__file__).resolve().parents[3];out=ROOT/'.context/amiga/fceux-player'
rom=Path(sys.argv[1]);frames=sys.argv[2];reference=None
for mode in ['no-input','input']:
 for binary in ['player-native','player-sanitize']:
  r=subprocess.run([out/binary,rom,frames,*(['input'] if mode=='input' else [])],check=True,capture_output=True,text=True)
  assert not r.stderr,r.stderr
  (out/(rom.stem+'-'+binary+'-'+mode+'.log')).write_text(r.stdout)
  if binary=='player-native':reference=r.stdout
  else:assert r.stdout==reference
 print('PASS',rom.name,mode,frames,'frames, ASan/selected UBSan adapter equivalence')
