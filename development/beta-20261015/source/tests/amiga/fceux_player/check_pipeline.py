#!/usr/bin/env python3
"""Exercise generated worker dispatch with explicit native architectural stubs.

Does not establish physical cache/MMU behaviour. check_arm.py exercises the
unchanged ARM presenter; live acceptance remains mandatory for the new command.
"""
from pathlib import Path
import re,subprocess
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'.context/amiga/fceux-player'
s=(OUT/'worker.cpp').read_text()
assert 'NES_FRAME_PRESENT' in s, 'Build --scheduling pipeline first'
s=re.sub(r'__asm__ volatile\(.*?\);', '', s, flags=re.S)
s=s.replace('uint32_t sctlr,midr,mpidr,seq=0;', 'uint32_t sctlr=0x1005,midr=0,mpidr=1,seq=0;')
s=s.replace('!(*(volatile uint32_t *)0xf8f00208u&1)', 'false')
a=s.index('static uint64_t ticks()');b=s.index('static void clean_range',a)
s=s[:a]+'static uint64_t ticks(){return fake_ticks();}\n'+s[b:]
a=s.index('static void clean_range');b=s.index('static void publish_frame',a)
s=s[:a]+'static void clean_range(volatile uint8_t *,uint32_t){}\n'+s[b:]
a=s.index(' for(uint32_t a=(uint32_t)d;');b=s.index(' barrier();',a)
s=s[:a]+s[b:]
s=re.sub(r'for\(uint32_t a=\(uint32_t\)\(base\+NES_SAVE\);.*?a\+=32\)', '', s)
s=s.replace('while(!stopped())', 'while(test_next_command(base))')
assert '__asm__' not in s
(OUT/'worker-under-test.cpp').write_text(s)
binary=OUT/'worker-protocol-test'
subprocess.run(['clang++','-std=c++14','-O2','-g','-fsanitize=address,undefined','-DNES_PLAYER','-DNES_DIRECT',
 '-I'+str(OUT),'-I'+str(ROOT/'amiga/mesen_bench'),'-I'+str(ROOT/'amiga/arm_debug'),'-I'+str(ROOT/'amiga/arm_debug/zz9000'),
 str(ROOT/'tests/amiga/fceux_player/worker_protocol.cpp'),'-o',str(binary)],check=True)
subprocess.run([binary],check=True)
