#!/usr/bin/env python3
"""Capture stereo PCM from the exact ARM core; no hardware timing claim."""
import argparse, csv, hashlib, json, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from test_console_jit_core import Machine
p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('build',type=Path);p.add_argument('output',type=Path);p.add_argument('--frames',type=int,default=600);a=p.parse_args()
m=Machine(a.build/'player/pocket.elf',a.build/'core/elf.py',96*1024**2,0x11000040)
rom=a.rom.read_bytes();m.u.mem_write(m.base+0x200000,rom)
assert m.call('ps_set_options',0)==1
assert m.call('nes_init',m.base+0x200000,len(rom),0,0)==0
total=0
with a.output.with_suffix('.pcm').open('wb') as pcm, a.output.with_suffix('.csv').open('w') as f:
 rows=csv.writer(f);rows.writerow(['frame','sample_frames','video_hash','pcm_sha256'])
 for frame in range(1,a.frames+1):
  assert m.call('nes_frame',0,0)==0
  n=m.call('nes_audio',m.base+0x1d0000);assert 0<=n<=1024
  ptr=struct.unpack('<I',m.u.mem_read(m.base+0x1d0000,4))[0]
  data=bytes(m.u.mem_read(ptr,n*4));pcm.write(data);total+=n
  rows.writerow([frame,n,m.call('nes_hash'),hashlib.sha256(data).hexdigest()])
  if frame%100==0:print('ARM frames',frame,'stereo sample frames',total,flush=True)
a.output.with_suffix('.json').write_text(json.dumps(dict(frames=a.frames,sample_frames=total,rate=32040,channels=2,rom_sha256=hashlib.sha256(rom).hexdigest(),elf_sha256=hashlib.sha256((a.build/'player/pocket.elf').read_bytes()).hexdigest(),pcm_sha256=hashlib.sha256(a.output.with_suffix('.pcm').read_bytes()).hexdigest(),execution='Unicorn Cortex-A9 model; not physical cache/performance validation'),indent=2)+'\n')
print('PASS capture',total,flush=True)
