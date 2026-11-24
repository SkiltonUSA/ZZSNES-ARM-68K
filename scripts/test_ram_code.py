#!/usr/bin/env python3
"""Run original self-modifying guest RAM through linked register dispatchers."""
import argparse,struct,hashlib
from pathlib import Path
from test_console_jit_core import Machine
from unicorn import UC_HOOK_BLOCK
from unicorn.arm_const import UC_ARM_REG_C1_C0_2,UC_ARM_REG_FPEXC

def cartridge(snes):
 code=bytearray();labels={};fixups=[]
 def emit(*v):code.extend(v)
 def label(name):labels[name]=len(code)
 def branch(op,name):emit(op,0);fixups.append((len(code)-1,name,True))
 def jump(name):emit(0x4c,0,0);fixups.append((len(code)-2,name,False))
 emit(0x78,0xd8,0xa2,0xff,0x9a) # SEI, CLD, stack
 emit(0xa9,0xe8,0x8d,0,2,0xa9,0x60,0x8d,1,2,0xa0,0)
 label('loop')
 emit(0xa2,0x7f,0x20,0,2,0xe0,0x80);branch(0xd0,'fail')
 emit(0xa9,0xca,0x8d,0,2,0xa2,0,0x20,0,2,0xe0,0xff);branch(0xd0,'fail')
 emit(0xa9,0xe8,0x8d,0,2,0xc8);branch(0xd0,'loop')
 emit(0xa9,0x5a,0x85,0x10);jump('done')
 label('fail');emit(0xa9,0xee,0x85,0x10)
 label('done');jump('done')
 for at,name,relative in fixups:
  value=labels[name]-at-1 if relative else 0x8000+labels[name]
  if relative:assert -128<=value<128;code[at]=value&255
  else:struct.pack_into('<H',code,at,value)
 rom=bytearray(32768);rom[:len(code)]=code
 if snes:
  rom[0x7fc0:0x7fd5]=b'ZZ RAM CODE TEST'.ljust(21,b' ')
  rom[0x7fd5:0x7fdc]=bytes([0x20,0,5,0,1,0,0])
  struct.pack_into('<HH',rom,0x7fdc,0xffff,0)
 for at in (0x7ffa,0x7ffc,0x7ffe):struct.pack_into('<H',rom,at,0x8000)
 return bytes(rom) if snes else b'NES\x1a'+bytes([2,1])+bytes(10)+rom+bytes(8192)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('build',type=Path);p.add_argument('--snes',action='store_true');p.add_argument('--output',type=Path,default=Path('.context/ram-code-test'));a=p.parse_args()
 a.output.mkdir(parents=True,exist_ok=True);rom=cartridge(a.snes)
 (a.output/('ram-code.sfc' if a.snes else 'ram-code.nes')).write_bytes(rom)
 for base in (0x12000040,0x150020c0):
  m=Machine(a.build/'player'/('pocket.elf' if a.snes else 'fceux.elf'),a.build/'core/elf.py',(96 if a.snes else 48)*1024**2,base)
  if not a.snes:
   from model_neon import correct_cpacr_read
   correct_cpacr_read(m)
  m.u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);m.u.reg_write(UC_ARM_REG_FPEXC,0x40000000);m.u.ctl_flush_tb()
  arena=(base+(0x3400000 if a.snes else 0x600000)+4095)&~4095;calls=[0]
  def entered(u,at,n,data):calls[0]+=1
  m.u.hook_add(UC_HOOK_BLOCK,entered,begin=arena,end=arena+2047)
  off=0x200000 if a.snes else 0x100000;m.u.mem_write(base+off,rom)
  init='nes_init' if a.snes else '_Z8nes_initPKhjPtS1_';frame='nes_frame' if a.snes else '_Z9nes_framehPFivE'
  assert m.call(init,base+off,len(rom),base+0x90000,base+0xb0000)==0
  for i in range(3):assert m.call(frame,0,0)==0
  if a.snes:marker=m.call('S9xGetByte',0x10);opcode=m.call('S9xGetByte',0x200)
  else:
   ram=struct.unpack('<I',m.u.mem_read(m.syms['RAM'],4))[0]
   marker=m.u.mem_read(ram+0x10,1)[0];opcode=m.u.mem_read(ram+0x200,1)[0]
  assert (marker,opcode)==(0x5a,0xe8),(hex(base),marker,opcode)
  assert calls[0]>=512,calls
  address=base+(0x880000 if a.snes else 0x500000);capacity=0x200000 if a.snes else 122880
  store='fc_state_store' if a.snes else '_Z14fc_state_storePhj'
  load='fc_state_load' if a.snes else '_Z13fc_state_loadPKhj'
  n=m.call(store,address,capacity);assert 0<n<=capacity
  state=bytes(m.u.mem_read(address,n))
  def next_output():
   assert m.call(frame,0,0)==0
   if a.snes:
    count=m.call('nes_audio',base+0x1d0000);ptr=struct.unpack('<I',m.u.mem_read(base+0x1d0000,4))[0]
    return m.call('nes_hash'),hashlib.sha256(m.u.mem_read(ptr,count*4)).hexdigest()
   return tuple(m.call(k) for k in ('_Z13fc_video_hashv','_Z13fc_audio_hashv','_Z10fc_samplesv'))
  expected=next_output()
  if a.snes:m.call('S9xSetByte',0xca,0x200)
  else:m.u.mem_write(ram+0x200,b'\xca')
  m.u.mem_write(address,state);assert m.call(load,address,n)==1
  restored=m.call('S9xGetByte',0x200) if a.snes else m.u.mem_read(ram+0x200,1)[0]
  assert restored==0xe8 and next_output()==expected,'RAM-code state restoration or PCM/video divergence'
  print('PASS','SNES' if a.snes else 'NES',hex(base),'512 live opcode changes and RAM-code state replay; translated blocks',calls[0],flush=True)
 print('Fixture SHA256',hashlib.sha256(rom).hexdigest())
if __name__=='__main__':main()
