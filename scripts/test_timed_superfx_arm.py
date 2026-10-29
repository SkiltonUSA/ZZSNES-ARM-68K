#!/usr/bin/env python3
"""Relocated A9 model: Legacy equivalence and timed state/PCM replay."""
import argparse
from pathlib import Path
from test_ppu_arm import load, frame


def save(machine):
    address=machine.base+0x880000
    size=machine.call('fc_state_store',address,0x200000)
    assert size
    return bytes(machine.u.mem_read(address,size))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom',type=Path)
    parser.add_argument('state',type=Path)
    parser.add_argument('--frames',type=int,default=30)
    parser.add_argument('--output-state',type=Path)
    args=parser.parse_args()
    if args.frames<1:parser.error('--frames must be positive')
    rom,state=args.rom.read_bytes(),args.state.read_bytes()
    baseline=Path('.context/ppu-baseline-build')
    candidate=Path('.context/timed-superfx-build')
    reference=load(baseline,0x12000040,rom,state)
    expected=[frame(reference) for _ in range(args.frames)]
    del reference
    first=None
    for base in (0x150020c0,0x12000040):
        machine=load(candidate,base,rom,state)
        assert [frame(machine) for _ in range(args.frames)]==expected
        print('PASS ARM Legacy video/PCM',args.rom.name,hex(base),args.frames,flush=True)
        legacy_state=save(machine)
        reference=load(baseline,0x18000040,rom,legacy_state)
        assert [frame(machine) for _ in range(args.frames)]==[frame(reference) for _ in range(args.frames)]
        del reference
        print('PASS candidate Legacy state resumes in previous player',args.rom.name,hex(base),flush=True)
        assert machine.call('ps_set_options',0x800000)==1
        for _ in range(5):frame(machine)
        checkpoint=save(machine)
        rows=[frame(machine) for _ in range(args.frames)]
        if first is None:first=rows
        assert rows==first
        del machine
        machine=load(candidate,base,rom,checkpoint)
        assert [frame(machine) for _ in range(args.frames)]==rows
        print('PASS ARM timed mode, relocated state and PCM replay',args.rom.name,hex(base),args.frames,flush=True)
        if args.output_state:args.output_state.write_bytes(checkpoint)
        del machine
if __name__=='__main__':main()
