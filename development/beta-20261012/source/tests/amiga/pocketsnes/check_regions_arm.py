#!/usr/bin/env python3
"""Bounded A9 PAL/Contra run, state relocation and guarded extended-height presentation.
Private ROMs supplied explicitly; no ROM data is written to tracked outputs.
"""
from pathlib import Path
import sys,struct,subprocess,importlib.util
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
ROOT=Path(__file__).resolve().parents[3];out=Path(sys.argv[1]).resolve()
spec=importlib.util.spec_from_file_location('pack',out/'elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
for name in sys.argv[2:]:
 rom=Path(name).read_bytes();saved=None;reference=None
 for base in [0x12000040,0x150020c0]:
  image,rel,_=pack.unpack((out/'pocket.elf').read_bytes())
  for off in rel:struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
  symbols={f[2]:int(f[0],16)+base for line in subprocess.check_output(['/opt/homebrew/opt/llvm/bin/llvm-nm',out/'pocket.elf'],text=True).splitlines() if len(f:=line.split())==3}
  u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(base&~4095,0x2002000);u.mem_write(base,bytes(image));u.mem_write(base+0x200000,rom)
  def call(name,*args):
   stack=base+0x1fffff0;end=base+0x1e0000;u.reg_write(UC_ARM_REG_SP,stack);u.reg_write(UC_ARM_REG_LR,end)
   for r,v in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):u.reg_write(r,v)
   for i,v in enumerate(args[4:]):u.mem_write(stack+i*4,struct.pack('<I',v))
   u.emu_start(symbols[name],end,count=250000000);assert u.reg_read(UC_ARM_REG_PC)==end,(name,hex(u.reg_read(UC_ARM_REG_PC)));return u.reg_read(UC_ARM_REG_R0)
  def frame():
   assert call('nes_frame',0,0)==0
   n=call('nes_audio',base+0x1e0100);assert 0<n<=1024;p=struct.unpack('<I',u.mem_read(base+0x1e0100,4))[0];return call('nes_hash'),bytes(u.mem_read(p,n*4))
  assert call('nes_init',base+0x200000,len(rom),0,0)==0
  pal=rom[0x7fd9]>=2;assert call('ps_frame_us')==(19873 if pal else 16689)
  if saved is None:
   for i in range(10):frame()
   n=call('fc_state_store',base+0x600000,0x100000);assert 500000<n<=0x100000;saved=bytes(u.mem_read(base+0x600000,n));reference=[frame() for _ in range(3)]
  else:u.mem_write(base+0x600000,saved);n=len(saved)
  assert call('fc_state_load',base+0x600000,n)==1;assert call('nes_frames')==10;assert [frame() for _ in range(3)]==reference
  print(f'PASS {Path(name).name} PAL={pal} base={base:08x} 10 frames + state replay, period={call("ps_frame_us")}',flush=True)
  # Synthetic completed frames make geometry guards independent of ROM startup.
  pixels=struct.pack('<61440H',*((i*13^0xa563)&65535 for i in range(61440)))
  u.mem_write(call('nes_pixels'),pixels)
  addr=0x02fff800;mapping=addr&~4095;capacity=0x100000;u.mem_map(mapping,capacity)
  l1=(base+0x100000+16383)&~16383;u.mem_write(l1,bytes(0x18000));u.mem_write(base+0x182000+60,struct.pack('<I',l1))
  for fmt in (3,4):
   for opt in (4,260,85,341):
    pitch=1296;span=239*pitch+1280;nonce=0x97ab301d
    u.mem_write(mapping,b'\xa5'*capacity);u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926));u.mem_write(base+0x180800+32,struct.pack('>I',opt))
    before=bytes(u.mem_read(mapping,capacity));assert call('nes_present',base,addr,pitch,fmt,nonce+1)==7;assert bytes(u.mem_read(mapping,capacity))==before
    assert call('nes_present',base,addr,pitch,fmt,nonce)==0
    result=bytes(u.mem_read(mapping,capacity));assert bytes(u.mem_read(l1,16384))==bytes(16384)
    expect=subprocess.check_output([out/'view-native',str(fmt),str(opt)],input=pixels)
    for y in range(240):
     row=addr-mapping+y*pitch;assert result[row:row+1280]==expect[y*1280:(y+1)*1280];assert result[row+1280:row+pitch]==b'\xa5'*16
  print('PASS 224/239 LUT/STMIA presentation, endian layouts, nonce guards, row guards and mapping revoke',flush=True)
