#!/usr/bin/env python3
"""SNES state replay across two A9 relocation bases, and per-ROM state isolation."""
from pathlib import Path
import sys,struct,subprocess,importlib.util
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
ROOT=Path(__file__).resolve().parents[3];out=ROOT/'.context/amiga/pocketsnes-library'
spec=importlib.util.spec_from_file_location('pack',out/'elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
rom=(ROOT/'.context/amiga/pocketsnes-game/input.sfc').read_bytes();saved=None;reference=None
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
 def frame(i):
  assert call('nes_frame',8 if 360<=i<368 else 0,0)==0
  n=call('nes_audio',base+0x1e0100);p=struct.unpack('<I',u.mem_read(base+0x1e0100,4))[0];pcm=bytes(u.mem_read(p,n*4));return call('nes_hash'),pcm
 assert call('nes_init',base+0x200000,len(rom),0,0)==0
 if saved is None:
  for i in range(400):frame(i)
  n=call('fc_state_store',base+0x600000,0x100000);assert 500000<n<=0x100000;saved=bytes(u.mem_read(base+0x600000,n));(out/'model-state.zzs').write_bytes(saved)
  assert any(saved[128:128+128*112*2]);reference=[frame(i) for i in range(400,430)]
 else:
  u.mem_write(base+0x600000,saved);n=len(saved)
 assert call('fc_state_load',base+0x600000,n)==1;assert call('nes_frames')==400
 actual=[frame(i) for i in range(400,430)];assert actual==reference,'Video/audio replay diverged'
 print(f'PASS base={base:08x} state={n} bytes; exact 30-frame video/stereo replay at restored frame400',flush=True)
 broken=bytearray(saved);broken[130]^=1;u.mem_write(base+0x600000,bytes(broken));assert call('fc_state_load',base+0x600000,n)==0;assert call('nes_frames')==430
 broken=bytearray(saved);broken[16]^=1;broken[28:32]=bytes(4);h=2166136261
 for v in broken:h=((h^v)*16777619)&0xffffffff
 broken[28:32]=struct.pack('>I',h);u.mem_write(base+0x600000,bytes(broken));assert call('fc_state_load',base+0x600000,n)==0
 print('PASS corrupted thumbnail and different-ROM header rejected without frame advance',flush=True)
 # Presenter: supported borders, both endian formats, guarded owned bitmap and revocation.
 addr=0x02fff800;mapping=addr&~4095;capacity=0x100000;u.mem_map(mapping,capacity)
 l1=(base+0x100000+16383)&~16383;u.mem_write(l1,bytes(0x18000));u.mem_write(base+0x182000+60,struct.pack('<I',l1))
 pixels=bytes(u.mem_read(call('nes_pixels'),122880));(out/'model-picture.raw').write_bytes(pixels)
 for fmt in (3,4):
  for opt in (0,2,4,6,8,5,21,85):
   pitch=1296;span=239*pitch+1280;nonce=0x97ab301d
   u.mem_write(mapping,b'\xa5'*capacity);u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926));u.mem_write(base+0x180800+32,struct.pack('>I',opt))
   before=bytes(u.mem_read(mapping,capacity));assert call('nes_present',base,addr,pitch,fmt,nonce+1)==7;assert bytes(u.mem_read(mapping,capacity))==before
   assert call('nes_present',base,addr,pitch,fmt,nonce)==0
   result=bytes(u.mem_read(mapping,capacity));assert bytes(u.mem_read(l1,16384))==bytes(16384)
   # Native converter generates independent address-space reference using exact source pixels.
   expect=subprocess.check_output([out/'view-native',str(fmt),str(opt)],input=pixels)
   assert len(expect)==320*240*4
   for y in range(240):
    row=addr-mapping+y*pitch;assert result[row:row+1280]==expect[y*1280:(y+1)*1280];assert result[row+1280:row+pitch]==b'\xa5'*16
 print('PASS all border variants, widescreen/masks, endian layouts, challenges, row guards and mapping revoke',flush=True)
print('PASS relocated state survives fresh Core1 address, no persisted runtime pointers')
