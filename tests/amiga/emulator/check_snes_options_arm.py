#!/usr/bin/env python3
"""Relocated ARM adapter options; model evidence, not physical cache proof."""
from pathlib import Path
import struct,subprocess,importlib.util,os
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
R=Path(__file__).resolve().parents[3];elf=R/'.context/expanded-build/player/pocket.elf'
spec=importlib.util.spec_from_file_location('pack',R/'development/expanded-20261019/core/elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
rom=bytearray(32768);rom[:11]=bytes.fromhex('af1100701a8f11007080fe');h=0x7fc0;rom[h:h+21]=b'ZZSNES BATTERY TEST  ';rom[h+21:h+26]=bytes([0x20,2,5,1,1]);rom[h+28:h+30]=b'\xff\xff';rom[h+61]=0x80
for base in [0x12000040,0x150020c0]:
 image,rel,_=pack.unpack(elf.read_bytes())
 for off in rel:struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
 symbols={f[2]:int(f[0],16)+base for line in subprocess.check_output([os.environ.get('LLVM_NM','llvm-nm'),elf],text=True).splitlines() if len(f:=line.split())==3}
 u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(base&~4095,0x3002000);u.mem_write(base,bytes(image));u.mem_write(base+0x200000,bytes(rom))
 def call(name,*args):
  end=base+0x1e0000;u.reg_write(UC_ARM_REG_SP,base+0x3000000-16);u.reg_write(UC_ARM_REG_LR,end)
  for r,v in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):u.reg_write(r,v)
  u.emu_start(symbols[name],end,count=250000000);assert u.reg_read(UC_ARM_REG_PC)==end,name;return u.reg_read(UC_ARM_REG_R0)
 assert call('ps_set_options',64)==1
 assert call('nes_init',base+0x200000,len(rom),0,0)==0
 assert call('nes_save',base+0x1e0100)==2048
 ram=struct.unpack('<I',u.mem_read(base+0x1e0100,4))[0];initial=u.mem_read(ram+17,1)[0]
 call('ps_set_pad2',256);call('ps_audio_feedback',0);assert call('nes_frame',1,0)==0
 assert u.mem_read(ram+17,1)[0]==(initial+1)&255
 assert call('S9xReadJoypad',0)&0x8000 and call('S9xReadJoypad',1)&128
 call('ps_audio_feedback',256|512|1);assert call('nes_frame',0,0)==0 and not call('ps_video_rendered')
 call('ps_audio_feedback',256|100);assert call('nes_frame',0,0)==0 and call('ps_video_rendered')
 text=base+0x880000;u.mem_write(text,b'7000115A\0');assert call('ps_cheats',text)==1;assert u.mem_read(ram+17,1)[0]==0x5a
 u.mem_write(text,b'\0');assert call('ps_cheats',text)==1;assert u.mem_read(ram+17,1)[0]==(initial+1)&255
 u.mem_write(text,b'7E12345A\0');assert call('ps_cheats',text)==1
 u.mem_write(text,b'INVALID\0');assert call('ps_cheats',text)==0
 u.mem_write(text,b'\0');assert call('ps_cheats',text)==1
 print(f'PASS relocated {base:08x}: SRAM execution, independent pads, automatic skip/recovery and cheat bounds',flush=True)
