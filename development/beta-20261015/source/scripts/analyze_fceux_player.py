#!/usr/bin/env python3
"""Validate bounded no-input player runs and report full frontend throughput."""
import argparse,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path);p.add_argument('rom',type=Path);p.add_argument('--input',action='store_true');p.add_argument('--no-audio-output',action='store_true');a=p.parse_args();s=a.log.read_text()
def fields(pattern):
 m=re.search(pattern,s);assert m,pattern;return list(map(int,m.groups()))
frames,elapsed,core=fields(r'ZZFCEUX END exit=0 frames=(\d+) elapsed_us=(\d+) core_total_us=(\d+)')
queued,flips=fields(r'FULLSCREEN_END queued=(\d+) flips=(\d+) pending=-1 ack_wait_us=\d+ fault=0');assert queued==flips==frames
q,sub,complete,high,stopped=fields(r'AUDIO_TASK queued=(\d+) submitted=(\d+) completed=(\d+) highwater=(\d+) error=0 stopped=([01])');assert stopped==(0 if a.no_audio_output else 1);assert q==sub==complete;assert (q==0 if a.no_audio_output else q>0)
samples,gaps=fields(r'AUDIO_END samples=(\d+) gaps=(\d+)');assert samples==q
challenges,verified,copy=fields(r'DIRECT_END challenges=(\d+) verified_frames=(\d+) copy_us=(\d+)');assert challenges==frames and verified>0
native=subprocess.check_output([ROOT/'.context/amiga/fceux-player/player-native',a.rom,str(frames),*(['input'] if a.input else [])],text=True).splitlines()
expected={int(m[1]):(int(m[2]),m[3],int(m[4])) for line in native if (m:=re.match(r'FRAME n=(\d+) cycles=(\d+) core_hash=([0-9a-f]+).* samples=(\d+)',line))}
assert a.no_audio_output or sum(r[2] for r in expected.values())==samples
checked=0
for m in re.finditer(r'FRAME n=(\d+) cycles=(\d+) core_hash=([0-9a-f]+)',s):
 n=int(m[1]);assert (int(m[2]),m[3])==expected[n][:2],n;checked+=1
assert checked>0
assert 'MAPPING PASS (eight nonce-derived words)' in s and 'EXIT epilogue=52455431' in s
assert re.search(r'MMU_RESTORE phase=3 register_mismatches=00000000 .* witness_bad=0 ',s)
assert 'LAUNCHER exit=0; owned memory released' in s
regs=re.findall(r'REGISTER n=(\d+) original=([0-9a-f]+) restored=([0-9a-f]+)',s);assert len(regs)==14 and all(x==y for n,x,y in regs)
result={'frames':frames,'scripted_start':a.input,'audio_output':not a.no_audio_output,'displayed_fps_including_readback_and_audio_drain':frames*1e6/elapsed,'elapsed_us':elapsed,'core_mean_ms':core/frames/1000,'audio_samples_completed':samples,'audio_gap_observations':gaps,'audio_highwater':high,'verified_framebuffers':verified,'native_sampled_frames_match':checked,'arm_present_mean_ms':copy/frames/1000,'clean_return':True,'audio_note':'AHI completion verified; audible quality requires owner confirmation; gaps are completed-predecessor observations'}
print(json.dumps(result,indent=2))
