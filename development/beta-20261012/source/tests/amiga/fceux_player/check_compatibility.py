#!/usr/bin/env python3
"""Original NROM layout/audio fixtures, no commercial cartridge downloads."""
from pathlib import Path
import subprocess,sys,json
root=Path(__file__).resolve().parents[3];sys.path.insert(0,str(root/'scripts'))
from make_nes_demo import build
out=root/'.context/amiga/fceux-player';results=[]
for banks in (1,2):
 for mirror in (0,1):
  r=bytearray(build(player_test=True));r[6]=mirror # audio fixture, no unsupported battery
  if banks==2:r=r[:16]+r[16:16400]*2+r[16400:];r[4]=2
  path=out/f'original-nrom{banks*16}-{mirror}.nes';path.write_bytes(r)
  subprocess.run([sys.executable,root/'tests/amiga/fceux_player/check_native.py',path,'120'],check=True)
  results.append({'fixture':path.name,'prg_kib':banks*16,'mirroring':mirror,'frames':120,'modes':['no-input','scripted Start'],'native_and_sanitizer':'pass'})
(out/'compatibility.json').write_text(json.dumps(results,indent=2)+'\n')
print('PASS four original audio-enabled NROM layouts; no general mapper compatibility claim')
