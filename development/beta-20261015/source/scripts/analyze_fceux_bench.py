#!/usr/bin/env python3
"""Validate an actual hardware log against native FCEUX; then report timings."""
import argparse,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path);p.add_argument('rom',type=Path);a=p.parse_args()
s=a.log.read_text()
m=re.search(r'FCEUX END exit=0 frames=(\d+) elapsed_us=(\d+) core_total_us=(\d+) samples=(\d+) nonzero=(\d+)',s);assert m,'successful summary missing'
frames,elapsed,core,samples,nz=map(int,m.groups());assert frames>0
rows=re.findall(r'^(FRAME n=\d+ cycles=\d+ video=[0-9a-f]{8} audio=[0-9a-f]{8} samples=\d+ nonzero=\d+) ticks=(\d+)$',s,re.M)
assert len(rows)==frames
native=subprocess.check_output([ROOT/'.context/amiga/fceux-bench/native-fixed',a.rom,str(frames)],text=True).splitlines()
assert [r[0] for r in rows]==native[:-1],'hardware/native emulation mismatch'
assert f'samples={samples} nonzero={nz} ' in native[-1]
assert 'MAPPING PASS (eight nonce-derived words)' in s
assert 'EXIT epilogue=52455431' in s
assert re.search(r'MMU_RESTORE phase=3 register_mismatches=00000000 .* witness_bad=0 ',s)
assert 'LAUNCHER exit=0; owned memory released' in s
regs=re.findall(r'REGISTER n=(\d+) original=([0-9a-f]+) restored=([0-9a-f]+)',s);assert len(regs)==14
assert all(x==y for n,x,y in regs)
rate=int(re.search(r'CLOCK timer_hz=(\d+)',s)[1]);ticks=[int(r[1]) for r in rows]
assert 320000000<=rate<=345000000
result={'frames':frames,'samples':samples,'nonzero_samples':nz,'timer_hz':rate,'native_all_frames_match':True,
 'core_mean_ms':core/frames/1000,'core_equivalent_fps':frames*1e6/core,'headless_elapsed_ms':elapsed/1000,'headless_throughput_fps':frames*1e6/elapsed,
 'core_frame_min_ms':min(ticks)*1000/rate,'core_frame_max_ms':max(ticks)*1000/rate,'frames_over_ntsc_16_639ms':sum(t*1e6/rate>16639 for t in ticks),
 'clean_return':True,'note':'Core throughput and headless throughput are not displayed FPS; no AHI playback'}
print(json.dumps(result,indent=2))
