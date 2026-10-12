#!/usr/bin/env python3
"""Read hardware cadence logs; distinguish emulation from presentation rates."""
import argparse,json,re
from pathlib import Path

def analyze(path,audio_rate=32040):
 text=path.read_text();m=re.search(r'CADENCE_RUN frames=(\d+) elapsed_us=(\d+) shown=(\d+) omitted=(\d+) rebases=(\d+) late_max=(\d+) max_streak=(\d+) period_us=(\d+)',text)
 assert m,path
 r=dict(zip(['frames','elapsed_us','shown','omitted','rebases','late_max_us','max_streak','period_us'],map(int,m.groups())))
 assert r['shown']+r['omitted']==r['frames'] and r['max_streak']<=2
 r.update(file=path.name,emulated_fps=r['frames']*1e6/r['elapsed_us'],presentations_per_second=r['shown']*1e6/r['elapsed_us'],omitted_percent=100*r['omitted']/r['frames'])
 a=re.search(r'AUDIO_TASK queued=(\d+) submitted=(\d+) completed=(\d+) highwater=(\d+) error=(\d+) stopped=(\d+)',text)
 assert a and a[1]==a[2]==a[3] and a[5]=='0' and a[6]=='1',path
 r['stereo_pairs']=int(a[1]);r['audio_highwater']=int(a[4]);r['gap_indicators']=int(re.search(r'AUDIO_END samples=\d+ gaps=(\d+)',text)[1])
 t=re.search(r'FRAME_TIMING stage=5 count=(\d+) mean_us=(\d+) max_us=(\d+) bins=([0-9,]+)',text)
 assert t,path
 bins=list(map(int,t[4].split(',')));r['host_ack']={'count':int(t[1]),'mean_us':int(t[2]),'max_us':int(t[3]),'over_20ms':bins[4]+bins[5]}
 points=[tuple(map(int,m.groups())) for m in re.finditer(r'CADENCE_PROGRESS frames=(\d+) elapsed_us=(\d+) samples=(\d+) submitted=(\d+) completed=(\d+) gaps=(\d+) flips=(\d+) omitted=(\d+)',text)]
 if len(points)>1:
  first,last=points[0],points[-1];dt=(last[1]-first[1])/1e6
  r['steady']={'seconds':dt,'emulated_fps':(last[0]-first[0])/dt,'presentations_per_second':(last[6]-first[6])/dt,'gaps_added':last[5]-first[5],
   'produced_pcm_seconds_minus_wall_seconds':(last[2]-first[2])/audio_rate-dt}
 r['clean_exit']=all(x in text for x in ['LAUNCHER exit=0; owned memory released','register_mismatches=00000000','witness_bad=0','fault=0'])
 assert r['clean_exit'],path
 return r
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('logs',type=Path,nargs='+');p.add_argument('--output',type=Path);a=p.parse_args()
 data=[analyze(f) for f in a.logs]
 if a.output:a.output.write_text(json.dumps(data,indent=2)+'\n')
 for r in data:print(f"{r['file']}: emulation {r['emulated_fps']:.3f} fps; presentation {r['presentations_per_second']:.3f}/s; omitted {r['omitted']}; gaps {r['gap_indicators']}; >20ms host ACKs {r['host_ack']['over_20ms']}")
