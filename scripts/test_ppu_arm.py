#!/usr/bin/env python3
"""Compare ARM raster variants from a private 32-bit checkpoint; no timing claim."""
import argparse
import hashlib
from pathlib import Path
import struct
from test_console_jit_core import Machine


def load(build, base, rom, state):
    machine = Machine(build / 'player/pocket.elf', build / 'core/elf.py', 0x6000000, base)
    machine.u.mem_write(base + 0x200000, rom)
    assert machine.call('nes_init', base + 0x200000, len(rom), 0, 0) == 0
    machine.u.mem_write(base + 0x880000, state)
    assert machine.call('fc_state_load', base + 0x880000, len(state)) == 1
    return machine


def frame(machine):
    # GSU-heavy frames can exceed the general helper's 50-million budget.
    assert machine.call('nes_frame', 0, 0, limit=500000000) == 0
    count = machine.call('nes_audio', machine.base + 0x1d0000)
    pointer = struct.unpack('<I', machine.u.mem_read(machine.base + 0x1d0000, 4))[0]
    return (machine.call('nes_hash'), count,
            hashlib.sha256(machine.u.mem_read(pointer, count * 4)).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('state', type=Path)
    parser.add_argument('--baseline', type=Path, default=Path('.context/ppu-baseline-build'))
    parser.add_argument('--candidate', type=Path, default=Path('.context/ppu-all-build'))
    parser.add_argument('--frames', type=int, default=30)
    args = parser.parse_args()
    if args.frames < 1:
        parser.error('--frames must be positive')
    rom, state = args.rom.read_bytes(), args.state.read_bytes()
    # These are model addresses only, never physical ZZ9000 allocations.
    expected = None
    for build, base in ((args.baseline, 0x12000040),
                        (args.candidate, 0x150020c0), (args.candidate, 0x12000040)):
        machine = load(build, base, rom, state)
        rows = [frame(machine) for _ in range(args.frames)]
        if expected is None:
            expected = rows
        assert rows == expected, (build, hex(base))
        print('PASS ARM video/stereo PCM', build.name, hex(base), len(rows), flush=True)


if __name__ == '__main__':
    main()
