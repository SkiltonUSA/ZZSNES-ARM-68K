#!/usr/bin/env python3
"""Compare isolated raster candidates with baseline, using synthetic/user input."""
import argparse
import os
from pathlib import Path
import subprocess
from compatibility_native import build

ROOT = Path(__file__).resolve().parents[1]
MODES = ('baseline', 'fixed', 'sprite', 'ppu', 'all')


def run(exe, args, log):
    with log.open('w') as stream:
        subprocess.run([str(exe), *map(str, args)], check=True, stdout=stream,
                       stderr=subprocess.STDOUT,
                       env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))


def compare(result, baseline):
    if result.read_bytes() != baseline.read_bytes():
        for index, (actual, expected) in enumerate(zip(result.read_text().splitlines(), baseline.read_text().splitlines())):
            if actual != expected:
                raise AssertionError(f'{result.name}, record {index}: {actual} != {expected}')
        raise AssertionError(f'{result.name}: trace length mismatch')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, action='append', default=[])
    parser.add_argument('--frames', type=int, default=3600)
    parser.add_argument('--profile', action='store_true', help='Also compare instrumented baseline and record operation counts')
    args = parser.parse_args()
    if args.frames < 1:
        parser.error('--frames must be positive')
    output = ROOT / '.context/ppu-review'
    output.mkdir(parents=True, exist_ok=True)
    for mode in MODES + (('profile',) if args.profile else ()):
        directory = ROOT / ('.context/ppu-fixture-' + mode)
        fixture = build(ROOT / 'development/ppu-20261029/tests/raster_fixture.c',
                        directory, fx3=True, expanded=True, ppu=mode)
        trace = output / ('fixture-' + mode + '.trace')
        run(fixture, [trace], output / ('fixture-' + mode + '.log'))
        compare(trace, output / 'fixture-baseline.trace')
        print('PASS synthetic sprite, blend and PPU window comparison:', mode, flush=True)
        if args.rom:
            game = build(ROOT / 'development/ppu-20261029/tests/game_trace.c',
                         directory, fx3=True, expanded=True, ppu=mode)
            for index, rom in enumerate(args.rom):
                trace = output / f'game-{index}-{mode}.trace'
                run(game, [rom.resolve(), args.frames, trace],
                    output / f'game-{index}-{mode}.log')
                compare(trace, output / f'game-{index}-baseline.trace')
                print(f'PASS {args.frames} video/PCM frames: ROM {index}, {mode}', flush=True)


if __name__ == '__main__':
    main()
