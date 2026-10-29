import json,re,statistics
from pathlib import Path
out=Path(__file__).resolve().parent
rows=[]
for mode in (0,1):
 for p in sorted((out/('fullscreen' if mode==0 else 'window')).glob('*.log')):
  t=p.read_text(encoding='latin1')
  assert 'END exit=0 frames=660' in t
  assert 'DIRECT_END challenges=660 verified_frames=1' in t
  assert 'register_mismatches=00000000' in t and 'witness_bad=0' in t and 'owned memory released' in t
  a=re.search(r'AUDIO_TASK queued=(\d+) submitted=(\d+) completed=(\d+).*error=0 stopped=1',t)
  assert a and a[1]==a[2]==a[3]
  if mode==0:assert 'FULLSCREEN_END queued=660 flips=660 pending=-1' in t
  if p.name.startswith('snes'):assert 'FRAME_RENDER rendered=660 skipped=0 options=0' in t
  s=re.search(r'SCHEDULE fixed=1 audit=0 capture=0 warmup=60 frames=600 total_us=(\d+)',t);assert s
  rows.append(dict(mode='fullscreen' if not mode else 'window',run=p.stem,steady_seconds=int(s[1])/1e6,gaps=int(re.search(r'AUDIO_END.*gaps=(\d+)',t)[1]),pcm=int(a[1])))
summary=[]
for mode in ('fullscreen','window'):
 for console in ('nes','snes'):
  groups={kind:[r for r in rows if r['mode']==mode and r['run'].startswith(console+'-'+kind+'-')] for kind in ('ref','audio')}
  if not all(len(v)==2 for v in groups.values()):continue
  times={k:statistics.mean(r['steady_seconds'] for r in v) for k,v in groups.items()}
  summary.append(dict(mode=mode,console=console,mean_seconds=times,reduction_percent=100*(1-times['audio']/times['ref']),runs=groups))
(out/'results.json').write_text(json.dumps(dict(runs=rows,summary=summary),indent=2)+'\n')
for s in summary:print(s['mode'],s['console'],s['mean_seconds'],'reduction',round(s['reduction_percent'],3),'gaps',{k:[r['gaps'] for r in v] for k,v in s['runs'].items()})
