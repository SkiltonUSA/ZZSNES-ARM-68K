#!/usr/bin/env python3
"""Private-ROM regression: unchanged Legacy output and timed save/PCM replay."""
import argparse
import os
from pathlib import Path
import subprocess
from compatibility_native import build

ROOT = Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom',type=Path,nargs='+')
    parser.add_argument('--frames',type=int,default=3600)
    args=parser.parse_args()
    if args.frames<1:parser.error('--frames must be positive')
    output=ROOT/'.context/timed-superfx-review';output.mkdir(exist_ok=True)
    source=ROOT/'development/timed-superfx-20261029/tests/game.c'
    for mode,timed,option in [('baseline',False,0),('legacy',True,0),('timed',True,0x800000)]:
        exe=build(source,ROOT/('.context/timed-superfx-game-'+mode),fx3=True,expanded=True,timed_fx=timed)
        for index,rom in enumerate(args.rom):
            trace=output/f'{index}-{mode}.trace'
            with (output/f'{index}-{mode}.log').open('w') as log:
                subprocess.run([str(exe),str(rom.resolve()),str(args.frames),str(trace),str(option)],
                               check=True,stdout=log,stderr=subprocess.STDOUT,
                               env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
            if mode=='legacy':assert trace.read_bytes()==(output/f'{index}-baseline.trace').read_bytes()
            print('PASS',mode,rom.name,args.frames,'frames + 60-frame state/PCM replay',flush=True)
if __name__=='__main__':main()
