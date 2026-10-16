#!/usr/bin/env python3
"""Original SNES ROM: show the checkerboard only after executing FX3 MERGE.
No commercial ROM input. Failed checks loop on a blank screen.
"""
import argparse
from pathlib import Path
import build_mesen_snes_system as fixture

def make(out):
    out.mkdir(parents=True, exist_ok=True)
    fixture.OUT = out
    rom = fixture.fixture()
    code = bytearray([0x78, 0x18, 0xfb, 0xe2, 0x20])
    branches = []
    def write(address, value):
        code.extend([0xa9, value, 0x8d, address & 255, address >> 8])
    def equal(address, value):
        code.extend([0xaf, address & 255, (address >> 8) & 255, address >> 16, 0xc9, value, 0xd0, 0])
        branches.append(len(code)-1)
    equal(0x703b, 0x52)
    code.extend([0xa9, 0, 0x8f, 0, 0, 0x71])
    write(0x7034, 0)
    write(0x701e, 0)
    write(0x701f, 0x83)
    equal(0x701e, 0)
    equal(0x701f, 0)
    equal(0x710000, 255)
    equal(0x710010, 0)
    equal(0x710031, 255)
    # Marker for host-native checks; then run the original PPU/DMA fixture.
    code.extend([0xa9, 0xf3, 0x8f, 0xf0, 0x1f, 0x7e, 0x4c, 0, 0x80])
    fail = len(code)
    code.extend([0x80, 0xfe])
    for pos in branches:
        offset = fail-(pos+1)
        assert 0 <= offset <= 127
        code[pos] = offset
    rom[0x300:0x306] = bytes([0xf0, 3, 0, 0x70, 0, 1])
    rom[0x400:0x400+len(code)] = code
    rom[0x7fd6] = 0x17
    rom[0x7ffc:0x7ffe] = bytes([0, 0x84])
    rom[0x7fdc:0x7fe0] = bytes(4)
    checksum = (sum(rom)+0x1fe) & 65535
    rom[0x7fdc:0x7fe0] = (checksum ^ 65535).to_bytes(2, 'little') + checksum.to_bytes(2, 'little')
    path = out/'FX3-Instructions.sfc'
    path.write_bytes(rom)
    return path

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('out', type=Path)
    print(make(p.parse_args().out))
