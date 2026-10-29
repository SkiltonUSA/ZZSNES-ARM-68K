"""Relocated ARM scaling checks; not physical cache/scanout or audio proof."""
import importlib.util,json,struct,sys
from pathlib import Path
from unicorn.arm_const import *
root=Path(__file__).resolve().parents[1]
for repo,snes in [(root.name, json.loads((root/'docs/current-build.json').read_text())['console']=='ZZSNES')]:
 r=root
 spec=importlib.util.spec_from_file_location('engine',r/'tests/amiga/emulator/check_current_arm.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 m.block=0x6000000 if snes else 0x3000000
 if snes:
  spec=importlib.util.spec_from_file_location('pack',r/'.context/ax-release-build/core/elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack);m.unpack=pack.unpack
 m.raw=struct.pack('<61440H',*[(i*17)&(65535 if snes else 511)for i in range(61440)])
 elf=r/'.context/ax-release-build/player'/('pocket.elf'if snes else'fceux.elf')
 for base in [0x12000040,0x150020c0]:
  u,call,addr,mapping,l1,counts=m.engine(elf,base)
  mapped=0x200000 if snes else 0x100000;u.mem_map(mapping+mapped,0x400000-mapped)
  cmd=base+m.control+0x800
  for fmt in [3,4]:
   for scan in [0,2]:
    reference=None
    for scale in [1,2,3]:
     pitch=1280*scale+16;span=(240*scale-1)*pitch+1280*scale;nonce=0x12345678
     u.mem_write(mapping,b'\xa5'*0x400000);u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926))
     u.mem_write(cmd+32,struct.pack('>IIII',16,scan,1,scale))
     assert call('nes_present',base,addr,pitch,fmt,nonce)==0,(repo,scale)
     actual=bytes(u.mem_read(addr,span))
     if scale==1:reference=[actual[y*pitch:y*pitch+1280]for y in range(240)]
     for y in range(240*scale):
      row=reference[y//scale];expected=b''.join(row[x:x+4]*scale for x in range(0,1280,4))
      assert actual[y*pitch:y*pitch+1280*scale]==expected,(repo,fmt,scan,scale,y)
      if y<240*scale-1:assert actual[y*pitch+1280*scale:(y+1)*pitch]==b'\xa5'*16
     assert bytes(u.mem_read(mapping,addr-mapping))==b'\xa5'*(addr-mapping)
     assert bytes(u.mem_read(addr+span,32))==b'\xa5'*32
     assert bytes(u.mem_read(l1,16384))==bytes(16384)
    # Reject bad nonce, excessive scale and cancellation without touching the frame.
    for stop,scale,challenge,expected in [(0,3,nonce+1,7),(1,3,nonce,2),(0,4,nonce,6)]:
     u.mem_write(base+m.control+128,struct.pack('>I',stop));u.mem_write(cmd+44,struct.pack('>I',scale))
     u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926));before=bytes(u.mem_read(addr,span))
     assert call('nes_present',base,addr,pitch,fmt,challenge)==expected
     assert bytes(u.mem_read(addr,span))==before and bytes(u.mem_read(l1,16384))==bytes(16384)
    u.mem_write(base+m.control+128,bytes(4))
    print('PASS',repo,hex(base),'format',fmt,'scan',scan,'scaled pixels, guards, cancellation and mapping restoration',flush=True)
