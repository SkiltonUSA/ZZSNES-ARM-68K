#!/usr/bin/env python3
"""Validate and summarize physical SNES system/profile logs; never reports game FPS."""
import argparse,json,re,statistics
from pathlib import Path

def analyze(path):
    text=path.read_text()
    for required in [
        'VERIFY checked=6 mismatch=0 cycles=277071 hash=1e1e29c5 errors=0 unsupported=0',
        'SYSTEM master=2093734 spc=199650 pcm=2cfa6c7d samples=3119',
        'CANCEL PASS active_work=1','MESEN_SNES_SYSTEM PASS',
        'EXIT epilogue=52455431','MMU_RESTORE phase=3 register_mismatches=00000000',
        'witness_bad=0','LAUNCHER exit=0; owned memory released',
    ]:
        assert required in text,(path,required)
    regs=re.findall(r'REGISTER n=(\d+) original=([0-9a-f]+) restored=([0-9a-f]+)',text)
    assert len(regs)==14 and {int(r[0]) for r in regs}==set(range(14)) and all(a==b for _,a,b in regs)
    hz=int(re.search(r'TIMER estimated_hz=(\d+)',text)[1]);assert 320000000<=hz<=345000000
    samples=[dict((k,int(v,16) if k=='hash' else int(v)) for k,v in re.findall(r'(\w+)=(\w+)',line)) for line in text.splitlines() if line.startswith('SAMPLE ')]
    assert len(samples)==11 and [s['n'] for s in samples]==list(range(11))
    for s in samples:
        assert s['warmup']==(s['n']==0) and s['frames']==6 and s['cycles']==277071 and s['hash']==0x1e1e29c5
    result={'scope':'Original six-frame startup fixture; no RTG presentation or audio playback; not game FPS',
        'timer_hz':hz,'warm_samples':10,'mean_frame_us':statistics.mean(s['core_us']/6 for s in samples[1:])}
    rows=[dict((k,int(v)) for k,v in re.findall(r'(\w+)=(\d+)',line)) for line in text.splitlines() if line.startswith('PROFILE ')]
    if rows:
        assert len(rows)==11 and [r['n'] for r in rows]==list(range(11))
        for r,s in zip(rows,samples):
            assert r['ppu_ticks']+r['spc_ticks']+r['residual_ticks']==s['ticks']
            assert r['ppu_calls']>0 and r['spc_calls']>0
        result['stages']={}
        for stage in ['ppu','spc','residual']:
            ticks=statistics.mean(r[stage+'_ticks'] for r in rows[1:])
            result['stages'][stage]={'mean_frame_us':ticks*1e6/hz/6,'percent':100*ticks/statistics.mean(s['ticks'] for s in samples[1:])}
        result['calls_per_batch']={k:sorted(set(r[k] for r in rows)) for k in ['ppu_calls','spc_calls']}
        result['measurement_note']='Coarse nested exclusive scopes. Residual includes CPU/bus/scheduling and instrumentation overhead; not isolated CPU time.'
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('logs',type=Path,nargs='+');a=p.parse_args()
    print(json.dumps({path.name:analyze(path) for path in a.logs},indent=2))
