#!/usr/bin/env python3
"""Count modeled ARM instructions from a private checkpoint; not hardware timing."""
import argparse
import bisect
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import subprocess

from test_ppu_arm import load, frame
from unicorn import UC_HOOK_BLOCK


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('rom', type=Path)
    parser.add_argument('state', type=Path)
    parser.add_argument('--frames', type=int, default=30)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.frames < 1:
        parser.error('--frames must be positive')
    elf = args.build / 'player/pocket.elf'
    machine = load(args.build, 0x12000040, args.rom.read_bytes(), args.state.read_bytes())
    symbols = []
    listing = subprocess.check_output(
        [os.environ.get('LLVM_NM', 'llvm-nm'), '-S', '--defined-only', str(elf)], text=True)
    for line in listing.splitlines():
        fields = line.split()
        if len(fields) == 4 and fields[2].lower() == 't':
            symbols.append((int(fields[0], 16) + machine.base, int(fields[1], 16), fields[3]))
    symbols.sort()
    starts = [item[0] for item in symbols]
    instructions, entries, lookup = Counter(), Counter(), {}

    def count_block(uc, address, size, data):
        if address not in lookup:
            index = bisect.bisect_right(starts, address) - 1
            symbol = symbols[index] if index >= 0 else (0, 0, 'unknown')
            lookup[address] = symbol if address < symbol[0] + symbol[1] else (0, 0, 'unknown')
        start, _, name = lookup[address]
        instructions[name] += size // 4  # This build uses ARM state, not Thumb.
        if address == start:
            entries[name] += 1

    machine.u.hook_add(UC_HOOK_BLOCK, count_block)
    machine.u.ctl_flush_tb()  # Include blocks translated while loading the state.
    outputs = [frame(machine) for _ in range(args.frames)]
    report = {
        'metric': 'modeled ARM block instruction slots; not A9 cycles or FPS',
        'frames': args.frames,
        'elf_sha256': hashlib.sha256(elf.read_bytes()).hexdigest(),
        'output_trace_sha256': hashlib.sha256(json.dumps(outputs).encode()).hexdigest(),
        'total': sum(instructions.values()),
        'functions': [{'name': name, 'instructions': value, 'entries': entries[name]}
                      for name, value in instructions.most_common()],
    }
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('Profiled', args.frames, 'frames;', report['total'], 'modeled instruction slots')


if __name__ == '__main__':
    main()
