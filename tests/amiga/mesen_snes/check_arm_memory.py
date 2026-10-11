#!/usr/bin/env python3
"""Exercise built scalar memory routines with strict ARM data-access bounds."""
import argparse,struct,subprocess
from check_system_arm import packer
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_MEM_READ
from unicorn.arm_const import UC_CPU_ARM_CORTEX_A9,UC_ARM_REG_SP,UC_ARM_REG_LR,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_PC
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('elf',type=Path);a=p.parse_args()
base=0x12000040;image,rel,_=packer.unpack(a.elf.read_bytes())
for off in rel:struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
symbols={f[2]:int(f[0],16)+base for line in subprocess.check_output(['/opt/homebrew/opt/llvm/bin/llvm-nm','-n',a.elf],text=True).splitlines() if len(f:=line.split())==3}
u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(base&~4095,0x402000);u.mem_write(base,bytes(image))
src=base+0xe0000;dst=base+0xf0000;source=bytes((i*37+19)&255 for i in range(512));u.mem_write(src,source)
expected={};bad=[]
def access(u,kind,address,size,value,user):
    if size>1 and address%size:bad.append(('unaligned',address,size))
    if src<=address<src+512:
        if kind!=UC_MEM_READ or not expected['s']<=address or address+size>expected['s']+expected['n']:bad.append(('source',address,size))
    if dst<=address<dst+512:
        if kind==UC_MEM_READ or not expected['d']<=address or address+size>expected['d']+expected['n']:bad.append(('destination',address,size))
    if bad:u.emu_stop()
u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
count=0
for n in [*range(34),63,64,65,127,128,129,255,256,257]:
 for d in range(8):
  for s in range(8):
   for fun in ['memcpy','memset']:
    if fun=='memset' and s:continue
    expected={'s':src+8+s,'d':dst+8+d,'n':n};u.mem_write(dst,b'\xac'*512)
    for reg,value in [(UC_ARM_REG_SP,base+0x3ffff0),(UC_ARM_REG_LR,base+0x3f0000),(UC_ARM_REG_R0,expected['d']),(UC_ARM_REG_R1,expected['s'] if fun=='memcpy' else 0x135),(UC_ARM_REG_R2,n)]:u.reg_write(reg,value)
    u.emu_start(symbols[fun],base+0x3f0000,count=100000)
    assert not bad,(fun,n,d,s,bad)
    assert u.reg_read(UC_ARM_REG_PC)==base+0x3f0000 and u.reg_read(UC_ARM_REG_R0)==expected['d']
    want=bytearray(b'\xac'*512);want[8+d:8+d+n]=source[8+s:8+s+n] if fun=='memcpy' else b'\x35'*n
    assert u.mem_read(dst,512)==want,(fun,n,d,s)
    count+=1
assert u.mem_read(src,512)==source
print(f'PASS ARM memory: {count} calls; exact read/write bounds, alignment, bytes, canaries, return value')
