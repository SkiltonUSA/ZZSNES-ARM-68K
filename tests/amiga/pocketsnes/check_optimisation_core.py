"""Byte-exact video/PCM comparison of two relocated builds; no hardware timing claim."""
import sys,struct,subprocess,importlib.util,os
from pathlib import Path
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
root=Path(__file__).resolve().parents[3];baseline,candidate,rom_path=map(Path,sys.argv[1:4]);frames=int(sys.argv[4])if len(sys.argv)>4 else 120
spec=importlib.util.spec_from_file_location('pack',root/'development/beta-20261015/source/core/elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
rom=rom_path.read_bytes()
def engine(elf,base):
 image,rel,_=pack.unpack(elf.read_bytes())
 for off in rel:struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
 sym={f[2]:int(f[0],16)+base for line in subprocess.check_output([os.environ.get('LLVM_NM','llvm-nm'),elf],text=True).splitlines()if len(f:=line.split())==3}
 u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(base&~4095,0x2002000);u.mem_write(base,bytes(image));u.mem_write(base+0x200000,rom)
 def call(name,*args):
  sp=base+0x1fffff0;end=base+0x1e0000;u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,end)
  for r,v in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):u.reg_write(r,v)
  for i,v in enumerate(args[4:]):u.mem_write(sp+i*4,struct.pack('<I',v))
  u.emu_start(sym[name],end,count=250000000);assert u.reg_read(UC_ARM_REG_PC)==end,name
  return u.reg_read(UC_ARM_REG_R0)
 assert call('nes_init',base+0x200000,len(rom),0,0)==0
 def frame(i):
  assert call('nes_frame',8 if 360<=i<368 else 128 if i>400 else 0,0)==0
  pixels=bytes(u.mem_read(call('nes_pixels'),122880));n=call('nes_audio',base+0x1e0100);p=struct.unpack('<I',u.mem_read(base+0x1e0100,4))[0]
  return pixels,bytes(u.mem_read(p,n*4))
 return frame
for base,n in [(0x12000040,frames),(0x150020c0,6)]:
 old=engine(baseline,base);new=engine(candidate,base)
 for i in range(n):
  assert old(i)==new(i),('video/PCM mismatch',i,hex(base))
  if (i+1)%120==0:print('PASS',hex(base),i+1,'frames video/PCM exact',flush=True)
 print('PASS relocated core equivalence',hex(base),n,rom_path.name,flush=True)
