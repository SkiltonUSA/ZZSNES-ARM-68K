#!/usr/bin/env python3
"""Execute the clipped-tile pixel/depth oracle on a Cortex-A9 model."""
import argparse,os,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import UC_CPU_ARM_CORTEX_A9,UC_ARM_REG_SP,UC_ARM_REG_LR,UC_ARM_REG_PC,UC_ARM_REG_R0,UC_ARM_REG_C1_C0_2,UC_ARM_REG_FPEXC
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--core',type=Path,default=Path('.context/clipped-flip-build/core'))
p.add_argument('--output',type=Path,default=Path('.context/clipped-arm-oracle'))
p.add_argument('--compiler',default='arm-none-eabi-gcc')
a=p.parse_args();r=Path(__file__).resolve().parents[1];o=a.output.resolve();o.mkdir(parents=True,exist_ok=True);core=a.core.resolve()
flags=['-O2','-ffunction-sections','-fdata-sections','-fno-strict-aliasing','-fwrapv','-mfloat-abi=soft','-mcpu=cortex-a9','-marm','-mno-unaligned-access','-D__LIBRETRO__','-DUSE_SA1','-D__OLD_RASTER_FX__','-DPOCKET_ARM','-DCLIPPED_ARM_PROBE']
flags+=['-I'+str(path) for path in (core,core/'src',core/'libretro',core/'libretro/libretro-common/include',r/'development/beta-20261015/source/amiga/mesen_bench')]
for name,path in [('tile',core/'src/tile.c'),('globals',core/'src/globals.c'),('fixture',r/'tests/amiga/emulator/clipped_tiles.c')]:
 subprocess.run([a.compiler,*flags,'-c',str(path),'-o',str(o/(name+'.o'))],check=True)
llvm=Path(os.environ.get('LLVM_BIN','/opt/homebrew/opt/llvm/bin'))
subprocess.run([os.environ.get('LD_LLD','/opt/homebrew/opt/lld/bin/ld.lld'),'--gc-sections','-Ttext=0x10000','-e','main',str(o/'tile.o'),str(o/'globals.o'),str(o/'fixture.o'),str(core/'libc.a'),'-o',str(o/'fixture.elf')],check=True)
subprocess.run([str(llvm/'llvm-objcopy'),'-O','binary',str(o/'fixture.elf'),str(o/'fixture.bin')],check=True)
symbols={parts[2]:int(parts[0],16) for line in subprocess.check_output([str(llvm/'llvm-nm'),str(o/'fixture.elf')],text=True).splitlines() if len(parts:=line.split())==3}
u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
# Synthetic model addresses, never a physical ZZ9000 allocation.
u.mem_map(0x10000,0x2000000);u.mem_write(0x10000,(o/'fixture.bin').read_bytes())
u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);u.reg_write(UC_ARM_REG_FPEXC,0x40000000) # archived memcpy uses VFP
u.reg_write(UC_ARM_REG_SP,0x1ff0000);u.reg_write(UC_ARM_REG_LR,0x1fe0000)
u.emu_start(symbols['main'],0x1fe0000,count=3000000000)
assert u.reg_read(UC_ARM_REG_PC)==0x1fe0000,'oracle instruction budget exhausted'
assert u.reg_read(UC_ARM_REG_R0)==0,'pixel/depth oracle failed'
print('PASS Cortex-A9 clipped tiles: 6336 pixel/depth cases; no physical timing claim')
