#!/usr/bin/env python3
"""Summarize bounded ZZSNES host audio traces (not scanout timestamps)."""
import argparse,json,re,statistics
from pathlib import Path

def summary(path):
 text=path.read_text()
 rows=[list(map(int,l.split()[1:])) for l in text.splitlines() if l.startswith('AT_FRAME ')]
 acks=[list(map(int,l.split()[1:])) for l in text.splitlines() if l.startswith('AT_ACK ')]
 expected_acks=1200 if 'GAME_AUDIO_END' in text else 1201
 assert len(rows)==1200 and len(acks)==expected_acks and 'overflow=0' in text,path
 assert [r[0] for r in rows]==list(range(1200)),path
 intervals=[(b[0],(b[1]-a[1])&0xffffffff) for a,b in zip(acks,acks[1:]) if b[0]>=60]
 slow=[{'frame':f,'interval_us':dt,'stages_us':rows[f][1:6],
        'copy_us':rows[f][6]-rows[f-1][6],
        'gap_delta':rows[f][11]-rows[f-1][11]} for f,dt in intervals if dt>20000]
 def stats(values):
  return {'count':len(values),'mean':round(statistics.mean(values),1),'max':max(values),'over_20ms':sum(x>20000 for x in values)}
 m=re.search(r'AUDIO_TASK queued=(\d+) submitted=(\d+) completed=(\d+) highwater=(\d+) error=(\d+) stopped=(\d+)',text)
 assert m,path
 return {'file':path.name,'frames':len(rows),'host_ack_intervals':stats([d for _,d in intervals]),
         'stages_us':{k:stats([r[i+1] for r in rows[60:]]) for i,k in enumerate(['request','drain','audio','present','loop'])},
         'copy_us':stats([b[6]-a[6] for a,b in zip(rows,rows[1:]) if b[0]>=60]),
         'audio_task':dict(zip(['queued','submitted','completed','highwater','error','stopped'],map(int,m.groups()))),
         'audio_gaps':int(re.search(r'AUDIO_END samples=\d+ gaps=(\d+)',text)[1]),
         'slow_ack_events':slow,
         'gap_frames':[b[0] for a,b in zip(rows,rows[1:]) if b[11]>a[11]],
         'note':'Main-task observations, not scanout timestamps; worker counters sampled individually.'}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('logs',type=Path,nargs='+');p.add_argument('--output',type=Path)
 a=p.parse_args();results=[summary(f) for f in a.logs]
 if a.output:a.output.write_text(json.dumps(results,indent=2)+'\n')
 for r in results:
  s=r['host_ack_intervals'];print(f"{r['file']}: ACK mean {s['mean']} us, max {s['max']} us, >20ms {s['over_20ms']}; gaps {r['audio_gaps']}; copy mean {r['copy_us']['mean']} us")
if __name__=='__main__':main()
