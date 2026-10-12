#!/usr/bin/env python3
"""Relocated A9 instruction model, not a physical MMU/cache test."""
from pathlib import Path
import struct,subprocess,sys
import json
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_CPU_ARM_CORTEX_A9,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3,UC_ARM_REG_SP,UC_ARM_REG_LR,UC_ARM_REG_PC
ROOT=Path(__file__).resolve().parents[3];sys.path.insert(0,str(ROOT/'amiga/fceux_bench'))
from elf import unpack
out=ROOT/'.context/amiga/fceux-player';base=0x12000040;block=0x400000
ui=json.loads((out/'build.json').read_text()).get('scheduling')=='release-ui'
image,rel,_=unpack((out/'fceux.elf').read_bytes())
for offset in rel:struct.pack_into('<I',image,offset,struct.unpack_from('<I',image,offset)[0]+base)
symbols={p[2]:int(p[0],16)+base for line in subprocess.check_output(['/opt/homebrew/opt/llvm/bin/llvm-nm',out/'fceux.elf'],text=True).splitlines() if len(p:=line.split())==3}
u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(base&~4095,block+0x2000);u.mem_write(base,bytes(image))
rom=Path(sys.argv[1]).read_bytes();u.mem_write(base+0x100000,rom)
def access(u,kind,address,size,value,user):
 if size>1 and address%size:raise AssertionError(f'unaligned {address:x} size={size} PC={u.reg_read(UC_ARM_REG_PC):x}')
u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
def call(symbol,*args):
 u.reg_write(UC_ARM_REG_SP,base+block-4096-16);u.reg_write(UC_ARM_REG_LR,base+0xf0000)
 for r,v in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):u.reg_write(r,v)
 for i,v in enumerate(args[4:]):u.mem_write(base+block-4096-16+i*4,struct.pack('<I',v))
 u.emu_start(symbols[symbol],base+0xf0000,count=50000000)
 assert u.reg_read(UC_ARM_REG_PC)==base+0xf0000,(symbol,hex(u.reg_read(UC_ARM_REG_PC)))
 return u.reg_read(UC_ARM_REG_R0)

assert call('_Z8nes_initPKhjPtS1_',base+0x100000,len(rom),base+0x90000,base+0xb0000)==0
native=subprocess.check_output([out/'player-native',sys.argv[1],sys.argv[2],'input'],text=True).splitlines()
for i in range(int(sys.argv[2])):
 buttons=8 if 90<=i+1<100 else 0
 assert call('_Z9nes_framehPFivE',buttons,0)==0
 actual=f'FRAME n={i+1} cycles={call("_Z10nes_cyclesv")} core_hash={call("_Z8nes_hashv"):08x} raw={call("_Z13fc_video_hashv"):08x} audio={call("_Z13fc_audio_hashv"):08x} samples={call("_Z10fc_samplesv")} buttons={buttons}'
 assert actual==native[i],(actual,native[i]);print(actual,flush=True)
print('PASS A9 adapter/native, scripted Start input, aligned accesses',flush=True)
if ui:
 state=base+0xd1000
 n=call('_Z14fc_state_storePhj',state,32768);assert 32<n<=32768
 saved=bytes(u.mem_read(state,n));expected=[]
 for i in range(3):
  assert call('_Z9nes_framehPFivE',i*16,0)==0
  expected.append((call('_Z13fc_video_hashv'),call('_Z13fc_audio_hashv'),call('_Z9fc_cyclesv')))
 u.mem_write(state,saved);assert call('_Z13fc_state_loadPKhj',state,n)==1
 for i in range(3):
  assert call('_Z9nes_framehPFivE',i*16,0)==0
  assert (call('_Z13fc_video_hashv'),call('_Z13fc_audio_hashv'),call('_Z9fc_cyclesv'))==expected[i]
 print('PASS A9 state replay video/PCM/cycles, bounded state area')
if 'nes_present' in symbols:
 # Model the temporary table contents and actual ARM pixel stores; Unicorn
 # flat memory does NOT validate physical translation, coherence or P96.
 addr=0x02fff800;mapping=addr&~4095;capacity=0x100000
 u.mem_map(mapping,capacity)
 l1=(base+0x40000+16383)&~16383
 u.mem_write(l1,bytes(16384+8192));u.mem_write(base+0x82000+15*4,struct.pack('<I',l1))
 source=call('_Z10nes_pixelsv')
 palette=[int(x,16) for x in __import__('re').findall(r'0x([0-9a-fA-F]{8})',(out/'player_palette.h').read_text())]
 raw=struct.unpack('<61440H',u.mem_read(source,61440*2))
 for fmt,opt in ([(fmt,opt) for fmt in (3,4) for opt in (0,16,17,19,21,23,25,81,169)] if ui else [(fmt,0) for fmt in range(1,5)]):
  width=320 if ui else 256
  bpp=3 if fmt<3 else 4;pitch=width*bpp+16;span=239*pitch+width*bpp
  if ui:u.mem_write(base+0x80800+32,struct.pack('>I',opt))
  expected=bytearray(b'\xa5'*capacity)
  nonce=0x97ab301d
  u.mem_write(mapping,bytes(expected))
  u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926))
  # A bad challenge must not alter any target memory or retain mappings.
  before=bytes(u.mem_read(mapping,capacity))
  if ui:
   u.mem_write(base+0x80800+32,struct.pack('>I',255))
   assert call('nes_present',base,addr,pitch,fmt,nonce)==6
   assert bytes(u.mem_read(mapping,capacity))==before
   assert bytes(u.mem_read(l1,16384))==bytes(16384)
   u.mem_write(base+0x80800+32,struct.pack('>I',opt))
  assert call('nes_present',base,addr,pitch,fmt,nonce+1)==7
  assert bytes(u.mem_read(mapping,capacity))==before
  assert bytes(u.mem_read(l1,16384))==bytes(16384)
  assert call('nes_present',base,addr,pitch,fmt,nonce)==0
  for i in range(width*240):
   y,x=divmod(i,width);c=palette[raw[y*256+x]&511] if not ui else 0
   if ui:
    gamewidth=240 if opt&1 else 320;left=(320-gamewidth)//2;sx=(x-left)*256//gamewidth
    mx=((opt>>4)&3)*8;my=((opt>>6)&3)*8;theme=(opt>>1)&7
    if x>=left and x<left+gamewidth and mx<=sx<256-mx and my<=y<240-my:c=palette[raw[y*256+sx]&511]
    elif theme==0:c=0xff000000
    elif theme==1:c=0xffffffff
    elif theme==2:c=0xffc82b32 if 190<=y<196 else 0xff282828 if 28<y<160 and x%8<3 else 0xffb8b8b0
    elif theme==3:c=0xff8c2438 if y<24 or y>212 else 0xffac903c if 180<y<185 else 0xffeee4c8
    else:
     h=x*1973+y*9277+89173;h=((h^(h>>7))*1103515245)&0xffffffff;c=0xffe4e8ff if h%211==0 else 0xff101827
   r=(c>>16)&255;g=(c>>8)&255;b=c&255
   color=[(r,g,b),(b,g,r),(b,g,r,255),(255,r,g,b)][fmt-1]
   offset=addr-mapping+y*pitch+x*bpp
   expected[offset:offset+bpp]=bytes(color)
  assert bytes(u.mem_read(mapping,capacity))==expected
  assert bytes(u.mem_read(l1,16384))==bytes(16384)
  # Stop before copying retains challenge bytes and revokes the map.
  u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926))
  before=bytes(u.mem_read(mapping,capacity));u.mem_write(base+0x80080,struct.pack('>I',1))
  assert call('nes_present',base,addr,pitch,fmt,nonce)==2
  assert bytes(u.mem_read(mapping,capacity))==before
  assert bytes(u.mem_read(l1,16384))==bytes(16384)
  u.mem_write(base+0x80080,bytes(4))
 print('PASS ARM presentation: supported formats/options, both-end challenges, guard/padding bytes, cancellation, mapping revocation')
