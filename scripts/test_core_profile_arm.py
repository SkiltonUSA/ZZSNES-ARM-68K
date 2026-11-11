#!/usr/bin/env python3
"""Check profiler video/PCM transparency; the modeled timer stays at zero."""
import argparse
import struct
from pathlib import Path
from test_console_jit_core import Machine
from test_ppu_arm import frame


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('state', type=Path)
    parser.add_argument('--frames', type=int, default=30)
    args = parser.parse_args()
    if args.frames < 1:
        parser.error('--frames must be positive')
    root = Path(__file__).resolve().parents[1]
    expected = None
    rom, state = args.rom.read_bytes(), args.state.read_bytes()
    for name, base in [('ax-release-build', 0x12000040),
                       ('core-profile-build', 0x150020c0),
                       ('core-profile-build', 0x12000040)]:
        build = root / '.context' / name
        machine = Machine(build / 'player/pocket.elf', build / 'core/elf.py',
                          0x6000000, base)
        machine.u.mem_map(0xf8f00000, 4096)
        machine.u.mem_write(base + 0x200000, rom)
        assert machine.call('nes_init', base + 0x200000, len(rom), 0, 0) == 0
        machine.u.mem_write(base + 0x880000, state)
        assert machine.call('fc_state_load', base + 0x880000, len(state)) == 1
        rows = [frame(machine) for _ in range(args.frames)]
        if expected is None:
            expected = rows
        else:
            assert rows == expected
            counts = struct.unpack('<3I', machine.u.mem_read(machine.syms['ps_stage_calls'], 12))
            assert counts[1] and counts[2] == args.frames
            print('Stage calls:', counts)
        print('PASS same video/stereo PCM', name, hex(base), args.frames, flush=True)


if __name__ == '__main__':
    main()
