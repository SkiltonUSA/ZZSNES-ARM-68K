"""Regional timing/PCM and state replay in relocated A9 instruction models.
Uses only the project's original fixture; not a hardware speed measurement.
"""
import os,sys,struct,subprocess,importlib.util
from pathlib import Path
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
root=Path(__file__).resolve().parents[3]
elf=Path(sys.argv[1]);spec=importlib.util.spec_from_file_location('pack',root/'development/beta-20261015/source/core/elf.py')
pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
for base in [0x12000040,0x150020c0]:
 for pal in [False,True]:
  image,rel,_=pack.unpack(elf.read_bytes())
  for off in rel:struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
  symbols={f[2]:int(f[0],16)+base for line in subprocess.check_output([os.environ.get('LLVM_NM','llvm-nm'),elf],text=True).splitlines()if len(f:=line.split())==3}
  u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(base&~4095,0x2002000);u.mem_write(base,bytes(image))
  rom=bytearray((root/'artifacts/pocketsnes/System-Test.sfc').read_bytes());rom[0x7fd9]=2 if pal else 1;u.mem_write(base+0x200000,bytes(rom))
  def call(name,*args):
   sp=base+0x1fffff0;end=base+0x1e0000;u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,end)
   for r,v in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):u.reg_write(r,v)
   u.emu_start(symbols[name],end,count=250000000);assert u.reg_read(UC_ARM_REG_PC)==end,name
   return u.reg_read(UC_ARM_REG_R0)
  def frame():
   assert call('nes_frame',0,0)==0
   n=call('nes_audio',base+0x1e0100);p=struct.unpack('<I',u.mem_read(base+0x1e0100,4))[0]
   return call('nes_hash'),n,bytes(u.mem_read(p,n*4))
  assert call('nes_init',base+0x200000,len(rom),0,0)==0
  period=20000 if pal else 16689;assert call('ps_frame_us')==period
  samples=sum(frame()[1]for _ in range(100))
  assert abs(samples-100*32040*period/1000000)<=1,(pal,samples)
  n=call('fc_state_store',base+0x600000,0x100000);assert n>500000
  expected=[frame()for _ in range(10)]
  assert call('fc_state_load',base+0x600000,n)==1
  assert [frame()for _ in range(10)]==expected
  print('PASS',hex(base),'PAL'if pal else'NTSC','period',period,'100-frame PCM',samples,'state video/PCM replay',flush=True)
