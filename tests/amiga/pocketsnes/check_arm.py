#!/usr/bin/env python3
"""Run built PocketSNES system code under Unicorn, not hardware. Optional unicorn==2.1.4.

Reject unaligned data accesses, check relocation at two synthetic addresses,
exercise mid-test cancellation and a corrupted reference (must fail closed).
No physical address, XACP, cache or frame-rate claims follow from these tests.
"""
import argparse
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_CODE
from unicorn.arm_const import UC_CPU_ARM_CORTEX_A9, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_PC, UC_ARM_REG_CPSR

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'scripts'))
spec=importlib.util.spec_from_file_location('packer',ROOT/'amiga/pocketsnes/elf.py')
packer=importlib.util.module_from_spec(spec);spec.loader.exec_module(packer)

def run(elf,base,corrupt=False,cancel=False,profile_clock=False):
    image,rel,_=packer.unpack(elf.read_bytes())
    for off in rel:
        struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
    symbols={}
    for line in subprocess.check_output(['/opt/homebrew/opt/llvm/bin/llvm-nm','-n',elf],text=True).splitlines():
        fields=line.split()
        if len(fields)==3:symbols[fields[2]]=int(fields[0],16)+base
    uc=Uc(UC_ARCH_ARM,UC_MODE_ARM)
    uc.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
    uc.mem_map(base&~4095,0x1802000)
    uc.mem_write(base,bytes(image))
    uc.mem_write(symbols['_ZL6shared'],struct.pack('<I',base))
    if corrupt:
        uc.mem_write(symbols['pocket_rom']+0x212,b'\x01')
    if cancel:
        # mov r0,#0; bx lr. Hook replaces r0 on the third callback.
        uc.mem_write(base+0x1d0000,bytes.fromhex('0000a0e31eff2fe1'))
        calls=[]
        def stop_hook(u,address,size,user):
            calls.append(address)
            u.reg_write(UC_ARM_REG_R0,int(len(calls)>=3))
        uc.hook_add(UC_HOOK_CODE,stop_hook,begin=base+0x1d0004,end=base+0x1d0004)
    if profile_clock:
        uc.mem_write(base+0x1d0100,bytes.fromhex('0000a0e31eff2fe1'))
        timer_calls=[]
        def timer_hook(u,address,size,user):
            timer_calls.append(address)
            u.reg_write(UC_ARM_REG_R0,len(timer_calls)*100)
        uc.hook_add(UC_HOOK_CODE,timer_hook,begin=base+0x1d0104,end=base+0x1d0104)
    for reg,value in [(UC_ARM_REG_SP,base+0x17ffff0),(UC_ARM_REG_LR,base+0x17f0000),
            (UC_ARM_REG_R0,1),(UC_ARM_REG_R1,base+0x1e0000),
            (UC_ARM_REG_R2,base+0x1d0000 if cancel else 0),(UC_ARM_REG_R3,base+0x1d0100 if profile_clock else 0)]:
        uc.reg_write(reg,value)
    unaligned=[]
    def access(u,kind,address,size,value,user):
        if address<base or address+size>base+0x1800000 or (size>1 and address%size):
            unaligned.append((address,size,u.reg_read(UC_ARM_REG_PC)))
            u.emu_stop()
    uc.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
    try:uc.emu_start(symbols['mesen_test'],base+0x17f0000,count=200000000)
    except Exception:
        print('FAULT relative PC',hex(uc.reg_read(UC_ARM_REG_PC)-base),'CPSR',hex(uc.reg_read(UC_ARM_REG_CPSR)),'bytes',bytes(uc.mem_read(uc.reg_read(UC_ARM_REG_PC),8)).hex(),flush=True);raise
    assert not unaligned,unaligned
    assert uc.reg_read(UC_ARM_REG_PC)==base+0x17f0000,'CPU test did not return'
    status=uc.reg_read(UC_ARM_REG_R0)
    result=struct.unpack('<11I',uc.mem_read(base+0x1e0000,44))
    if corrupt:
        assert status==5 and result[1]>0,(status,result)
    elif cancel:
        assert status==2 and result[0]==2,(status,result)
    else:
        assert status==0 and result[:6]==(6,0,6,0x67711dc5,0,0),(status,result)
        assert result[7:]==(256,224,0x77d82d05,3208),result
    if profile_clock and not corrupt and not cancel:
        assert len(timer_calls)==12 and result[6]==600,(timer_calls,result)
    print(f'PASS ARM model base={base:08x} corrupt={corrupt} cancel={cancel} status={status} checked={result[0]}')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf',type=Path,nargs='?',default=ROOT/'.context/amiga/pocketsnes/pocket.elf')
    parser.add_argument('--profile-clock',action='store_true')
    a=parser.parse_args()
    run(a.elf,0x12000040,profile_clock=a.profile_clock)
    run(a.elf,0x150020c0,profile_clock=a.profile_clock)
    run(a.elf,0x12000040,corrupt=True,profile_clock=a.profile_clock)
    run(a.elf,0x12000040,cancel=True,profile_clock=a.profile_clock)
