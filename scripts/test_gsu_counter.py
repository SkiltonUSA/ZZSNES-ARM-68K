#!/usr/bin/env python3
"""Compare the actual old/new GSU loops at STOP and budget boundaries."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile
from prepare_gsu_counter import prepare

ROOT = Path(__file__).resolve().parents[1]


def loop(source):
    begin = source.index('static uint32 fx_run(')
    end = source.index('\n#ifdef FX_FUNCTION_TABLE', begin)
    return source[begin:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, action='append', default=[])
    parser.add_argument('--frames', type=int, default=3600)
    args = parser.parse_args()
    if args.frames < 1:
        parser.error('--frames must be positive')
    source = (ROOT / 'development/expanded-20261019/core/src/fxinst.c').read_text()
    with tempfile.TemporaryDirectory() as folder:
        directory = Path(folder)
        (directory / 'src').mkdir()
        (directory / 'src/fxinst.c').write_text(source)
        prepare(directory)
        candidate = (directory / 'src/fxinst.c').read_text()
        harness = r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef uint32_t uint32;
static struct {uint32 vCounter,vInstCount,go;} GSU;
static uint32 steps,stop_after,reads;
#define TF(flag) GSU.go
#define READR14 reads++
/* STOP is the only handler that writes the budget; full handlers are also
 * covered by per-frame game video/PCM/GSU-state comparisons. */
#define FX_STEP do {steps++;if(steps==stop_after){GSU.go=0;GSU.vCounter=0;GSU.vInstCount=0;}} while(0)
'''
        harness += loop(source).replace('fx_run(', 'reference(')
        harness += loop(candidate).replace('fx_run(', 'candidate(')
        harness += r'''
int main(void){
 for(unsigned active=0;active<2;active++)
  for(unsigned budget=0;budget<128;budget++)
   for(unsigned stop=0;stop<130;stop++){
    GSU.go=active;GSU.vCounter=123;GSU.vInstCount=7;steps=reads=0;stop_after=stop;
    uint32 result=reference(budget),counter=GSU.vCounter,inst=GSU.vInstCount;
    uint32 go=GSU.go,count=steps,read_count=reads;
    GSU.go=active;GSU.vCounter=123;GSU.vInstCount=7;steps=reads=0;
    assert(candidate(budget)==result);
    assert(GSU.vCounter==counter && GSU.vInstCount==inst && GSU.go==go);
    assert(steps==count && reads==read_count);
   }
 puts("PASS 33280 actual-loop cases: zero/exhausted budget, unsigned wrap, STOP, inactive GSU, return value and ROM read");
}
'''
        path = directory / 'test.c'
        path.write_text(harness)
        exe = directory / 'test'
        subprocess.run(['clang', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', str(path), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)

    if args.rom:
        from compatibility_native import build
        output = ROOT / '.context/gsu-counter-validation'
        for name, enabled in [('reference', False), ('candidate', True)]:
            exe = build(ROOT / 'development/gsu-counter-20261111/game_trace.c',
                        output / name, fx3=True, expanded=True, ppu='rows',
                        gsu_counter=enabled)
            for index, rom in enumerate(args.rom):
                trace = output / f'{index}-{name}.trace'
                subprocess.run([str(exe), str(rom.resolve()), str(args.frames), str(trace)],
                               check=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
                if enabled:
                    assert trace.read_bytes() == (output / f'{index}-reference.trace').read_bytes(), rom
                    print('PASS video/stereo PCM/GSU state', rom.name, args.frames, flush=True)


if __name__ == '__main__':
    main()
