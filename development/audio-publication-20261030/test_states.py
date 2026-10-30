#!/usr/bin/env python3
"""Exact stereo/video state replay in fresh relocated ARM instances."""
import argparse, hashlib, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'scripts'))
from test_console_jit_core import Machine
p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('build',type=Path);p.add_argument('--state',type=Path);p.add_argument('--frames',type=int,default=300);a=p.parse_args()
rom=a.rom.read_bytes();saved=a.state.read_bytes() if a.state else None
options=struct.unpack_from('>I',saved,64)[0] if saved else 0
reference=None
for base in (0x12000040,0x150020c0):
 m=Machine(a.build/'player/pocket.elf',a.build/'core/elf.py',96*1024**2,base)
 m.u.mem_write(base+0x200000,rom);assert m.call('ps_set_options',options)==1
 assert m.call('nes_init',base+0x200000,len(rom),0,0)==0
 def frame():
  assert m.call('nes_frame',0,0)==0
  n=m.call('nes_audio',base+0x1d0000);ptr=struct.unpack('<I',m.u.mem_read(base+0x1d0000,4))[0]
  return m.call('nes_hash'),n,bytes(m.u.mem_read(ptr,n*4))
 if saved is None:
  for _ in range(a.frames):frame()
  n=m.call('fc_state_store',base+0x880000,0x200000);assert 500000<n<0x200000
  saved=bytes(m.u.mem_read(base+0x880000,n));reference=[frame() for _ in range(30)]
 m.u.mem_write(base+0x880000,saved);assert m.call('fc_state_load',base+0x880000,len(saved))==1
 assert m.call('nes_frames')==struct.unpack_from('>I',saved,48)[0]
 actual=[frame() for _ in range(30)]
 if reference is None:reference=actual
 assert actual==reference,'Stereo/video replay mismatch'
 before=m.call('nes_frames');bad=bytearray(saved);bad[130]^=1;m.u.mem_write(base+0x880000,bytes(bad))
 assert m.call('fc_state_load',base+0x880000,len(bad))==0
 assert m.call('nes_frames')==before
 print('PASS fresh relocated stereo/video replay',hex(base),'state_version',struct.unpack_from('>I',saved,8)[0], 'state_bytes',len(saved),'corrupt-state rejection',flush=True)
print('PASS rom',hashlib.sha256(rom).hexdigest(),flush=True)
