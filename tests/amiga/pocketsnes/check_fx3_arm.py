#!/usr/bin/env python3
"""Original FX3 MMIO/instruction fixture on the relocated Cortex-A9 image.

Instruction emulation is not physical cache/scanout or commercial-game proof.
"""
from pathlib import Path
import argparse, importlib.util, os, struct, subprocess
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM
from unicorn.arm_const import *

ROOT = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('elf', type=Path)
a = p.parse_args()
spec = importlib.util.spec_from_file_location('pack', ROOT/'development/beta-20261015/source/core/elf.py')
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
rom = bytearray((i >> 16) ^ 0x5a for i in range(0x400000))
rom[0:2] = b'\x80\xfe'
rom[0x7fc0:0x7fd5] = b'ORIGINAL FX3 A9 TEST  '[:21]
rom[0x7fd5:0x7fda] = bytes([0x20, 0x17, 12, 7, 1])
rom[0x7fdc:0x7fe0] = b'\xff\xff\0\0'
rom[0x7ffc:0x7ffe] = b'\0\x80'
assert len(rom) == 0x400000

for base in (0x12000040, 0x150020c0):
    image, rel, _ = pack.unpack(a.elf.read_bytes())
    for off in rel:
        struct.pack_into('<I', image, off, struct.unpack_from('<I', image, off)[0] + base)
    symbols = {f[2]: int(f[0], 16) + base for line in subprocess.check_output(
        [os.environ.get('LLVM_NM', 'llvm-nm'), a.elf], text=True).splitlines()
        if len(f := line.split()) == 3}
    u = Uc(UC_ARCH_ARM, UC_MODE_ARM)
    u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
    u.mem_map(base & ~4095, 0x2002000)
    u.mem_write(base, bytes(image))
    u.mem_write(base + 0x200000, bytes(rom))

    def call(name, *args):
        end = base + 0x1e0000
        u.reg_write(UC_ARM_REG_SP, base + 0x1fffff0)
        u.reg_write(UC_ARM_REG_LR, end)
        for r, v in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), args):
            u.reg_write(r, v)
        u.emu_start(symbols[name], end, count=250000000)
        assert u.reg_read(UC_ARM_REG_PC) == end, name
        return u.reg_read(UC_ARM_REG_R0)

    def ptr(name):
        return struct.unpack('<I', u.mem_read(symbols[name], 4))[0]

    def put(value, address):
        call('S9xSetPPU', value, address)

    assert call('nes_init', base + 0x200000, len(rom), 0, 0) == 0
    assert u.mem_read(symbols['Fx3Active'], 1) == b'\x01'
    ram, code = ptr('SRAM'), ptr('ROM')
    assert bytes(u.mem_read(code, len(rom))) == bytes(rom)
    for mirror in range(4):
        assert call('S9xGetPPU', 0x703b + mirror*0x400) == 0x52
    for command in range(7):
        u.mem_write(ram, bytes([0x5a])*0x20000)
        u.mem_write(code + 0x200100, bytes([0xf0, command, 0, 0x70, 0, 1]))
        call('S9xResetSuperFX')
        put(0x60, 0x7034)
        put(0, 0x701e)
        put(1, 0x701f)
        assert call('S9xGetPPU', 0x701e) == call('S9xGetPPU', 0x701f) == 0
        assert call('S9xGetPPU', 0x7030) & 0x20 == 0
        assert call('S9xGetPPU', 0x7031) & 0x80 == 0
        expected = bytearray([0x5a])*0x20000
        if 3 <= command <= 5:
            for y in range((command-3)*9, (command-2)*9):
                for x in range(18):
                    start = 0x10000 + (y*20+x)*64
                    expected[start:start+64] = bytes(64)
                    for row in range(8):
                        expected[start+row*2] = expected[start+49+row*2] = 255
        assert bytes(u.mem_read(ram, len(expected))) == bytes(expected), command
    # Bounded execution must return to the host; CPU can still stop the GSU.
    call('S9xResetSuperFX')
    u.mem_write(code + 0x200100, bytes([5, 0xfe, 1]))
    put(0x60, 0x7034)
    put(0, 0x701e)
    put(1, 0x701f)
    assert call('S9xGetPPU', 0x7030) & 0x20
    put(0, 0x7030)
    assert call('S9xGetPPU', 0x7030) & 0x20 == 0
    print(f'PASS A9 base={base:08x}: FX3 VCR, intact 4 MiB ROM, MERGE 0..6, STOP and bounded execution', flush=True)
