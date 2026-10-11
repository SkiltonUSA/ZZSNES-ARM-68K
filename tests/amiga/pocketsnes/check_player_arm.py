#!/usr/bin/env python3
"""Relocated SNES player and locked-bitmap instruction model, not hardware."""
import sys,struct,subprocess,re
from pathlib import Path
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent))
from check_arm import packer,ROOT
out=ROOT/'.context/amiga/pocketsnes-player';rom=(ROOT/'.context/amiga/pocketsnes-game/input.sfc').read_bytes()
expected={}
for line in (out/'native.log').read_text().splitlines():
 if line.startswith('FRAME '):
  v={k:int(s,16 if k in ['padded','visible','pcm'] else 10) for k,s in re.findall(r'(\w+)=(\w+)',line)};expected[v['n']]=v
for base in [0x12000040,0x150020c0]:
 image,rel,_=packer.unpack((out/'pocket.elf').read_bytes())
 for off in rel:struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
 symbols={}
 for line in subprocess.check_output(['/opt/homebrew/opt/llvm/bin/llvm-nm',out/'pocket.elf'],text=True).splitlines():
  f=line.split()
  if len(f)==3:symbols[f[2]]=int(f[0],16)+base
 u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
 u.mem_map(base&~4095,0x1802000);u.mem_write(base,bytes(image));u.mem_write(base+0x190000,rom)
 def call(symbol,*args):
  stack=base+0x17ffff0;end=base+0x215000
  u.reg_write(UC_ARM_REG_SP,stack);u.reg_write(UC_ARM_REG_LR,end)
  for r,v in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):u.reg_write(r,v)
  for i,v in enumerate(args[4:]):u.mem_write(stack+i*4,struct.pack('<I',v))
  u.emu_start(symbols[symbol],end,count=100000000)
  assert u.reg_read(UC_ARM_REG_PC)==end,(symbol,hex(u.reg_read(UC_ARM_REG_PC)))
  return u.reg_read(UC_ARM_REG_R0)
 assert call('nes_init',base+0x190000,1,0,0)==1
 u.mem_write(base+0x190000,bytes([rom[0]^1]));assert call('nes_init',base+0x190000,len(rom),0,0)==1;u.mem_write(base+0x190000,rom[:1])
 assert call('nes_init',base+0x190000,len(rom),0,0)==0
 # mov r0,#1; bx lr: cancelled frame must not advance emulation.
 u.mem_write(base+0x214000,bytes.fromhex('0100a0e31eff2fe1'));assert call('nes_frame',0,base+0x214000)==2;assert call('nes_frames')==0
 pcm=2166136261;pairs=nonzero=0
 for n in range(600):
  buttons=8 if 360<=n<368 or 480<=n<488 else 0
  assert call('nes_frame',buttons,0)==0
  count=call('nes_audio',base+0x214100);assert count<=1024
  ptr=struct.unpack('<I',u.mem_read(base+0x214100,4))[0];pairs+=count
  for sample in struct.unpack('<'+str(count*2)+'H',u.mem_read(ptr,count*4)):
   pcm=((pcm^sample)*16777619)&0xffffffff;nonzero+=sample!=0
  if (n+1)%60==0:
   e=expected[n+1];assert(call('nes_hash'),pcm,pairs,nonzero)==(e['padded'],e['pcm'],e['pairs'],e['nonzero'])
   print(f'PASS base={base:08x} frame={n+1} padded={e["padded"]:08x} pcm={pcm:08x} stereo_pairs={pairs}',flush=True)
 # Flat Unicorn memory models actual presenter stores and table revocation,
 # not physical cache coherence, P96 ownership or framebuffer visibility.
 addr=0x02fff800;mapping=addr&~4095;capacity=0x100000;u.mem_map(mapping,capacity)
 l1=(base+0x100000+16383)&~16383;u.mem_write(l1,bytes(0xc000));u.mem_write(base+0x182000+60,struct.pack('<I',l1))
 src=call('nes_pixels');pixels=struct.unpack('<61440H',u.mem_read(src,122880))
 for fmt in (3,4):
  pitch=1040;span=239*pitch+1024;nonce=0x97ab301d
  u.mem_write(mapping,b'\xa5'*capacity);u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926))
  before=bytes(u.mem_read(mapping,capacity))
  assert call('nes_present',base,addr,pitch,1,nonce)==6
  assert call('nes_present',base,addr,pitch,fmt,nonce+1)==7
  assert bytes(u.mem_read(mapping,capacity))==before and bytes(u.mem_read(l1,16384))==bytes(16384)
  assert call('nes_present',base,addr,pitch,fmt,nonce)==0
  want=bytearray(b'\xa5'*capacity)
  for i,pixel in enumerate(pixels):
   y,x=divmod(i,256);r=pixel>>11;g=(pixel>>5)&63;b=pixel&31;r=r*8+r//4;g=g*4+g//16;b=b*8+b//4
   at=addr-mapping+y*pitch+x*4;want[at:at+4]=bytes((b,g,r,255) if fmt==3 else (255,r,g,b))
  assert bytes(u.mem_read(mapping,capacity))==want and bytes(u.mem_read(l1,16384))==bytes(16384)
  u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926));before=bytes(u.mem_read(mapping,capacity));u.mem_write(base+0x180080,struct.pack('>I',1))
  assert call('nes_present',base,addr,pitch,fmt,nonce)==2
  assert bytes(u.mem_read(mapping,capacity))==before and bytes(u.mem_read(l1,16384))==bytes(16384)
  u.mem_write(base+0x180080,bytes(4))
 print('PASS presentation colours, row padding, end challenges, cancellation and mapping revocation',flush=True)
print('PASS both relocated 600-frame stereo player models')
